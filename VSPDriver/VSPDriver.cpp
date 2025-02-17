// ********************************************************************
// VSPDriver - Driver root object
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************

// -- OS
#include <stdio.h>
#include <os/log.h>

#include <DriverKit/IOLib.h>
#include <DriverKit/IOTypes.h>
#include <DriverKit/IOService.h>
#include <DriverKit/IOUserClient.h>
#include <DriverKit/OSDictionary.h>
#include <DriverKit/OSData.h>

// -- My
#include "VSPDriver.h"
#include "VSPLogger.h"
#include "VSPController.h"
#include "VSPSerialPort.h"
#include "VSPUserClient.h"

#define LOG_PREFIX "VSPDriver"

#define kVSPSerialPortProperties "SerialPortProperties"
#define kVSPContollerProperties  "UserClientProperties"
#define kVSPDefaultPortCount 4
#define kVSPMaxumumPorts 16

// Driver instance state resource
struct VSPDriver_IVars {
    uint8_t        m_portCount = 0;              // number of allocated serial ports
    TVSPortItem**  m_serialPorts = nullptr;      // list of allocated serial ports
    uint8_t        m_portLinkCount = 0;          // number of serial port links
    TVSPLinkItem** m_portLinks = nullptr;        // list of serial port links
};

// --------------------------------------------------------------------
// Allocate internal resources
//
bool VSPDriver::init(void)
{
    bool result;
    
    VSPLog(LOG_PREFIX, "init called.\n");
    
    if (!(result = super::init())) {
        VSPLog(LOG_PREFIX, "free (super) falsed. result=%d\n", result);
        goto finish;
    }
    
    // Create instance state resource
    ivars = IONewZero(VSPDriver_IVars, 1);
    if (!ivars) {
        VSPLog(LOG_PREFIX, "Unable to allocate driver data.\n");
        result = false;
        goto finish;
    }
    
    VSPLog(LOG_PREFIX, "init finished.\n");
    return true;
    
finish:
    return result;
}

// --------------------------------------------------------------------
// Release internal resources
//
void VSPDriver::free(void)
{
    VSPLog(LOG_PREFIX, "free called.\n");
    
    // Release instance state resource
    IOSafeDeleteNULL(ivars, VSPDriver_IVars, 1);
    super::free();
}

// --------------------------------------------------------------------
// Start_Impl(IOService* provider)
//
kern_return_t IMPL(VSPDriver, Start)
{
    kern_return_t ret;
    
    VSPLog(LOG_PREFIX, "Start: called.\n");
    
    // sane check our driver instance vars
    if (!ivars) {
        VSPLog(LOG_PREFIX, "Start: Private driver instance is NULL\n");
        return kIOReturnInvalid;
    }
    
    // Start service instance (Apple style super call)
    ret = Start(provider, SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "Start(super): failed. code=%d\n", ret);
        return ret;
    }
    
    // Create 4 serial port instances with each IOSerialBSDClient as a child instance
    if ((ret = CreateSerialPort(provider, kVSPDefaultPortCount)) != kIOReturnSuccess) {
        goto finish;
    }
    
    // Register driver instance to IOReg
    if ((ret = RegisterService()) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "Start: RegisterService failed. code=%d\n", ret);
        goto finish;
    }
    
    VSPLog(LOG_PREFIX, "Start: driver started successfully.\n");
    return kIOReturnSuccess;
    
finish:
    Stop(provider, SUPERDISPATCH);
    return ret;
}

// --------------------------------------------------------------------
// Stop_Impl(IOService* provider)
//
kern_return_t IMPL(VSPDriver, Stop)
{
    kern_return_t ret;
    
    VSPLog(LOG_PREFIX, "Stop called.\n");
        
    // release allocated port links and port items.
    // Do not release VSPSerialPort instance itself!
    if (ivars) {
        // release all link items and the list
        if (ivars->m_portLinks && ivars->m_portLinkCount) {
            for (uint8_t i = 0; i < ivars->m_portLinkCount; i++) {
                IOSafeDeleteNULL(ivars->m_portLinks[i], TVSPLinkItem, 1);
            }
        }
        IOSafeDeleteNULL(ivars->m_portLinks, TVSPLinkItem*, ivars->m_portLinkCount);

        // release all port items and the list
        if (ivars->m_serialPorts && ivars->m_portCount) {
            for (uint8_t i = 0; i < ivars->m_portCount; i++) {
                ivars->m_serialPorts[i]->port->unlinkParent();
                IOSafeDeleteNULL(ivars->m_serialPorts[i], TVSPortItem, 1);
            }
        }
        IOSafeDeleteNULL(ivars->m_serialPorts, VSPSerialPort*, ivars->m_portCount);
    }
    
    // service instance (Apple style super call)
    if ((ret= Stop(provider, SUPERDISPATCH)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "Stop (suprt) failed. code=%d\n", ret);
    } else {
        VSPLog(LOG_PREFIX, "driver successfully removed.\n");
    }
    
    return ret;
}

// --------------------------------------------------------------------
// NewUserClient_Impl(uint32_t type, IOUserClient ** userClient)
//
kern_return_t IMPL(VSPDriver, NewUserClient)
{
    kern_return_t ret;
    
    VSPLog(LOG_PREFIX, "NewUserClient called.\n");

    if (!userClient) {
        VSPLog(LOG_PREFIX, "NewUserClient: Invalid argument.\n");
        return kIOReturnBadArgument;
    }
    
    if ((ret = CreateUserClient(GetProvider(), userClient)) != kIOReturnSuccess) {
        return ret;
    }
    
    VSPLog(LOG_PREFIX, "NewUserClient finished.\n");
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// Create given number of VSPSerialPort instances
//
kern_return_t VSPDriver::CreateSerialPort(IOService* provider, uint8_t count)
{
    kern_return_t ret;
    IOService* service;
    uint8_t offset = 0;
    uint8_t lastId = 0;
    
    VSPLog(LOG_PREFIX, "CreateSerialPort: create #%d VSPSerialPort from Info.plist.\n", count);

    // Allocate serial port list. Hold each allocated
    // instance of the VSPSerialPort object
    if (!ivars->m_serialPorts) {
        ivars->m_portCount = 0;
        ivars->m_serialPorts = IONewZero(TVSPortItem*, count);
        if (!ivars->m_serialPorts) {
            VSPLog(LOG_PREFIX, "CreateSerialPort: Out of memory.\n");
            return kIOReturnNoMemory;
        }
        offset = 0;
    }
    // expand current port list
    else {
        uint8_t portCount = ivars->m_portCount;
        TVSPortItem** oldl = ivars->m_serialPorts;
        TVSPortItem** newl = IONewZero(TVSPortItem*, portCount + 1);
        for (int i = 0; i < portCount; i++) {
            newl[i] = ivars->m_serialPorts[i];
            lastId = newl[i]->id;
            offset++;
        }
        IOSafeDeleteNULL(oldl, TVSPortItem*, portCount);
        ivars->m_serialPorts = newl;
    }
    
    // do it
    for (uint8_t i = offset; i < (offset + count) && i < kVSPMaxumumPorts; i++) {
        VSPLog(LOG_PREFIX, "CreateSerialPort: Create serial port %d.\n", lastId + 1);
        
        TVSPortItem* item = IONewZero(TVSPortItem, 1);
        if (!item) {
            VSPLog(LOG_PREFIX, "CreateSerialPort: Ouch out of memory.\n");
            IOSafeDeleteNULL(ivars->m_serialPorts, TVSPortItem*, count);
            return kIOReturnNoMemory;
        }

        // Create sub service object from UserClientProperties in Info.plist
        ret= Create(this, kVSPSerialPortProperties, &service);
        if (ret != kIOReturnSuccess || service == nullptr) {
            VSPLog(LOG_PREFIX, "CreateSerialPort: create [%d] failed. code=%d\n", count, ret);
            IOSafeDeleteNULL(ivars->m_serialPorts, TVSPortItem*, count);
            IOSafeDeleteNULL(item, TVSPortItem, 1);
            return ret;
        }
        
        VSPLog(LOG_PREFIX, "CreateSerialPort: check VSPSerialPort type.\n");
        
        // Sane check object type
        item->port = OSDynamicCast(VSPSerialPort, service);
        if (item->port == nullptr) {
            VSPLog(LOG_PREFIX, "CreateSerialPort: Cast to VSPSerialPort failed.\n");
            IOSafeDeleteNULL(ivars->m_serialPorts, TVSPortItem*, count);
            IOSafeDeleteNULL(item, TVSPortItem, 1);
            service->release();
            return kIOReturnInvalid;
        }
        
        item->id = ++lastId;
        item->port->setParent(this);                // set this as parent
        item->port->setPortIdentifier(item->id);    // set unique identifier
        item->flags = 0x00;

        // save instance for controller
        ivars->m_serialPorts[i] = item;
        ivars->m_portCount++;
    }
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// Create the 'VSP Controller' user client instances
//
kern_return_t VSPDriver::CreateUserClient(IOService* provider, IOUserClient** userClient)
{
    kern_return_t ret;
    IOService* service;
    
    VSPLog(LOG_PREFIX, "CreateUserClient: create VSP user client from Info.plist.\n");
    
    if (!userClient) {
        VSPLog(LOG_PREFIX, "CreateUserClient: Bad argument 'userClient' detected.\n");
        return kIOReturnBadArgument;
    }
    
    // Create sub service object from UserClientProperties in Info.plist
    ret= Create(this, kVSPContollerProperties, &service);
    if (ret != kIOReturnSuccess || service == nullptr) {
        VSPLog(LOG_PREFIX, "CreateUserClient: create failed. code=%d\n", ret);
        return ret;
    }
    
    VSPLog(LOG_PREFIX, "CreateUserClient: check VSPUserClient type.\n");
    
    // Sane check object type
    VSPUserClient* client;
    client = OSDynamicCast(VSPUserClient, service);
    if (client == nullptr) {
        VSPLog(LOG_PREFIX, "CreateUserClient: Cast to VSPUserClient failed.\n");
        service->release();
        return kIOReturnInvalid;
    }
    
    client->setParent(this);
    (*userClient) = client;
    
    VSPLog(LOG_PREFIX, "CreateUserClient: success.");
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// Check serial port id
//
bool VSPDriver::checkPortId(uint8_t id)
{
    return (id >= 1);
}

// --------------------------------------------------------------------
// Check serial port link id
//
bool VSPDriver::checkPortLinkId(uint8_t id)
{
    return (id >= 1);
}

// --------------------------------------------------------------------
// Return nummer of allocated serial ports
//
kern_return_t VSPDriver::getPortCount(uint8_t* count)
{
    if (count == nullptr) {
        return kIOReturnBadArgument;
    }
    
    (*count) = ivars->m_portCount;

    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// Return all IDs of all allocated serial ports
//
kern_return_t VSPDriver::getPortList(uint8_t* list, uint8_t count)
{
    if (!list) {
        return kIOReturnBadArgument;
    }
    
    if (ivars->m_portCount == 0 || !ivars->m_serialPorts) {
        return kIOReturnNotFound;
    }
    
    for(uint8_t i = 0; i < count && i < ivars->m_portCount; i++) {
        list[i] = ivars->m_serialPorts[i]->id;
    }
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// Return all IDs of all allocated serial ports
//
kern_return_t VSPDriver::getPortLinkList(uint64_t* list, uint8_t count)
{
    if (!list) {
        return kIOReturnBadArgument;
    }
    
    if (ivars->m_portLinkCount == 0 || ivars->m_portCount == 0) {
        return kIOReturnNotFound;
    }
    
    for(uint8_t i = 0; i < count && i < ivars->m_portLinkCount; i++) {
        const uint8_t lid = ivars->m_portLinks[i]->id;
        const uint8_t src = ivars->m_portLinks[i]->sourcePort.id;
        const uint8_t tgt = ivars->m_portLinks[i]->targetPort.id;
        list[i] = (((lid << 16) | (src << 8) | tgt) & 0x00ffffffL);
    }
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// Create new VSPSerialPort incstance (called by VSPUserClient)
//
kern_return_t VSPDriver::createPort()
{
    if (ivars->m_portCount >= MAX_SERIAL_PORTS) {
        VSPLog(LOG_PREFIX, "createPort: Maximum of %d serial ports reached.\n",
               ivars->m_portCount);
        return kIOReturnNoSpace;
    }
    return CreateSerialPort(GetProvider(), 1);
}

// --------------------------------------------------------------------
// Remove VSPSerialPort incstance by given portId
// (called by VSPUserClient)
//
kern_return_t VSPDriver::removePort(uint8_t portId)
{
    IOReturn ret;
    uint8_t count = ivars->m_portCount;
    uint8_t found = 0;
    
    if (count == 0) {
        return kIOReturnNotFound;
    }

    if (!checkPortId(portId)) {
        return kIOReturnBadArgument;
    }

    /* find serial port in link table and remove the link */
    if (ivars->m_portLinkCount) {
        for (uint8_t i = 0; i < ivars->m_portLinkCount; i++) {
            TVSPLinkItem* pli;
            if (!(pli = ivars->m_portLinks[i])) {
                continue;
            }
            if (pli->sourcePort.id == portId) {
                removePortLink(pli);
                break;
            }
            else if (pli->targetPort.id == portId) {
                removePortLink(pli);
                break;
            }
        }
    }

    // old port list to remove
    TVSPortItem** oldl = ivars->m_serialPorts;

    // new port link list
    TVSPortItem** newl;
    if (!(newl= IONewZero(TVSPortItem*, count - 1))) {
        VSPLog(LOG_PREFIX, "removePort: Out of memory.\n");
        return kIOReturnNoMemory;
    }

    /* find given serial port item and exclude this from new new list */
    for (uint8_t i = 0, j = 0; i < count; i++) {
        TVSPortItem* item;
        if (!(item = ivars->m_serialPorts[i])) {
            continue;
        }
        if (item->id == portId) {
            VSPLog(LOG_PREFIX, "removePort: Port id=%d found. Release VSPSerialPort\n", item->id);
            ret = item->port->Stop(GetProvider());
            if (ret == kIOReturnSuccess) {
                OSSafeReleaseNULL(item->port);
                IOSafeDeleteNULL(item, TVSPortItem, 1);
                found = 1;
            } else {
                VSPLog(LOG_PREFIX, "removePort: Releae port failed. code=%d\n", ret);
            }
        }
        else {
            TVSPortItem* nitm;
            if ((nitm = IONewZero(TVSPortItem, 1)) == nullptr) {
                VSPLog(LOG_PREFIX, "removePort: Out of memory\n");
                IOSafeDeleteNULL(newl, TVSPortItem*, count - 1);
                return kIOReturnNoMemory;
            }
            // deep copy instead pointers!
            memcpy(nitm, item, sizeof(TVSPortItem));
            newl[j++] = nitm;
        }
    }

    // remove serial port
    if (found) {
        ivars->m_serialPorts = newl;
        ivars->m_portCount--;
        // remove old items
        for(uint8_t i = 0; i < count; i++) {
            IOSafeDeleteNULL(oldl[i], TVSPortItem, 1);
        }
        // dealloc old list
        IOSafeDeleteNULL(oldl, TVSPortItem*, count);
        return kIOReturnSuccess; // done!
    }

    IOSafeDeleteNULL(newl, TVSPortItem*, count - 1);
    return kIOReturnNotFound;
}

// --------------------------------------------------------------------
// Create a link between two serial ports
//
kern_return_t VSPDriver::createPortLink(uint8_t sourceId, uint8_t targetId, void** link)
{
    if (!link || !checkPortId(sourceId) || !checkPortId(targetId)) {
        VSPLog(LOG_PREFIX, "createPortLink: Invalid arguments. srcId=%d tgtId=%d\n",
               sourceId, targetId);
        return kIOReturnBadArgument;
    }
    
    if (ivars->m_portLinkCount >= MAX_PORT_LINKS) {
        VSPLog(LOG_PREFIX, "createPortLink: Maximum of %d links reached. srcId=%d tgtId=%d\n",
               ivars->m_portLinkCount, sourceId, targetId);
        return kIOReturnNoSpace;
    }

    if (sourceId == targetId) {
        VSPLog(LOG_PREFIX, "createPortLink: Link of same ports prohibited. srcId=%d tgtId=%d\n",
               sourceId, targetId);
        return kIOReturnInvalid;
    }

    TVSPortItem* src = nullptr;
    TVSPortItem* tgt = nullptr;
    
    // check serial port list
    for (uint8_t i = 0; i < ivars->m_portCount; i++) {
        
        // skip invalid entry
        if (!ivars->m_serialPorts[i] || !ivars->m_serialPorts[i]->port) {
            continue;
        }
        
        const TVSPortItem* port = ivars->m_serialPorts[i];
        uint8_t linkId = 0;
        
        if (port->id == sourceId) {
            linkId = port->port->getPortLinkIdentifier();
            if (linkId == 0) {
                src = ivars->m_serialPorts[i];
            } else {
                VSPLog(LOG_PREFIX, "createPortLink: Source port %d already assigned to link %d.",
                       port->id, linkId);
                return kIOReturnBadArgument;
            }
        }

        if (port->id == targetId) {
            linkId = port->port->getPortLinkIdentifier();
            if (linkId == 0) {
                tgt = ivars->m_serialPorts[i];
            } else {
                VSPLog(LOG_PREFIX, "createPortLink: Target port %d already assigned to link %d.",
                       port->id, linkId);
                return kIOReturnBadArgument;
            }
        }
    }
    
    if (src == nullptr || tgt == nullptr) {
        VSPLog(LOG_PREFIX, "createPortLink: Can't find sourceId=%d or targetId=%d\n",
               sourceId, targetId);
        return kIOReturnNotFound;
    }

    uint8_t count = ivars->m_portLinkCount;
 
    // double check existing links
    if (count && ivars->m_portLinks) {
        for (uint8_t i = 0; i < count; i++) {
            // skip invalid entry
            if (!ivars->m_portLinks[i]) {
                continue;;
            }
            
            TVSPLinkItem* link = ivars->m_portLinks[i];
            
            if (link->sourcePort.id == src->id) {
                VSPLog(LOG_PREFIX, "createPortLink: Source port %d already assigned to link %d\n",
                       src->id, link->id);
                return kIOReturnBadArgument;
            }
            
            if (link->targetPort.id == tgt->id) {
                VSPLog(LOG_PREFIX, "createPortLink: Target port %d already assigned to link %d\n",
                       tgt->id, link->id);
                return kIOReturnBadArgument;
            }
        }
    }

    // create new link item
    TVSPLinkItem* item = nullptr;
    if (!(item = IONewZero(TVSPLinkItem, 1))) {
        VSPLog(LOG_PREFIX, "createPortLink: Out of memory!\n");
        return kIOReturnNoMemory;
    }
    
    // new list with additional space
    TVSPLinkItem** newl = nullptr;
    if (!(newl = IONewZero(TVSPLinkItem*, count + 1))) {
        VSPLog(LOG_PREFIX, "createPortLink: Out of memory!\n");
        IOSafeDeleteNULL(item, TVSPLinkItem, 1);
        return kIOReturnNoMemory;
    }
    
    /* copy existing links */
    if (count && ivars->m_portLinks) {
        for (uint8_t i = 0; i < count; i++) {
            if (ivars->m_portLinks[i] == nullptr) {
                continue;
            }
            TVSPLinkItem* nitm;
            if (!(nitm = IONewZero(TVSPLinkItem, 1))) {
                VSPLog(LOG_PREFIX, "createPortLink: Out of memory!\n");
                IOSafeDeleteNULL(item, TVSPLinkItem, 1);
                IOSafeDeleteNULL(newl, TVSPLinkItem*, count);
                return kIOReturnNoMemory;
            }
            // deep copy instead pointers!
            memcpy(nitm, ivars->m_portLinks[i], sizeof(TVSPLinkItem));
            newl[i] = nitm;
        }
    }
    
    // old list to remove
    TVSPLinkItem** oldl = ivars->m_portLinks;

    // setup new port link item
    item->id = count + 1;
    item->sourcePort.id = src->id;
    item->sourcePort.port = src->port;
    item->sourcePort.flags = 0x01;
    src->port->setPortLinkIdentifier(item->id);
    
    item->targetPort.id = tgt->id;
    item->targetPort.port = tgt->port;
    item->targetPort.flags = 0x01;
    tgt->port->setPortLinkIdentifier(item->id);
    
    VSPLog(LOG_PREFIX, "createPortLink add linkId=%d with ports %d <-> %d\n",
           item->id, src->id, tgt->id);

    // set new link item
    newl[count] = item;
    
    // return new item
    (*link) = item;
    
    // update global members
    ivars->m_portLinks = newl;
    ivars->m_portLinkCount++;

    // could be NULL
    if (oldl != NULL) {
        // remove old items
        for(uint8_t i = 0; i < count; i++) {
            IOSafeDeleteNULL(oldl[i], TVSPLinkItem, 1);
        }
        // dealloc old list
        IOSafeDeleteNULL(oldl, TVSPLinkItem*, count);
    }
    
    return kIOReturnSuccess; // done!
}

// --------------------------------------------------------------------
// Removes prior created serial port link
//
kern_return_t VSPDriver::removePortLink(void *link)
{
    if (!link) {
        VSPLog(LOG_PREFIX, "removePortLink: Invalid argument.");
        return kIOReturnBadArgument;
    }
    
    if (ivars->m_portLinkCount == 0 || !ivars->m_portLinks) {
        VSPLog(LOG_PREFIX, "removePortLink: No serial port links available.");
        return kIOReturnNotFound;
    }
            
    TVSPLinkItem* item = reinterpret_cast<TVSPLinkItem*>(link);
    uint8_t count = ivars->m_portLinkCount;
    uint8_t found = 0;
    
    // old port list to remove
    TVSPLinkItem** oldl = ivars->m_portLinks;

    // new port link list
    TVSPLinkItem** newl;
    if (!(newl= IONewZero(TVSPLinkItem*, count - 1))) {
        VSPLog(LOG_PREFIX, "removePortLink: Out of memory.\n");
        return kIOReturnNoMemory;
    }

    /* find given link item and exclude this from new new list */
    for (uint8_t i = 0, j = 0; i < count; i++) {
        if (item->id == ivars->m_portLinks[i]->id) {
            VSPLog(LOG_PREFIX, "removePortLink: Link Id=%d found :)\n", item->id);
            if (item->sourcePort.port) {
                item->sourcePort.port->setPortLinkIdentifier(0);
            }
            if (item->targetPort.port) {
                item->targetPort.port->setPortLinkIdentifier(0);
            }
            found = 1;
        } else {
            newl[j++] = ivars->m_portLinks[i];
        }
    }

    // remove port link
    if (found) {
        ivars->m_portLinks = newl;
        ivars->m_portLinkCount--;
        IOSafeDeleteNULL(item, TVSPLinkItem, 1);
        IOSafeDeleteNULL(oldl, TVSPLinkItem*, count);
        return kIOReturnSuccess; // done!
    }

    IOSafeDeleteNULL(newl, TVSPLinkItem*, count - 1);
    return kIOReturnNotFound;
}

// --------------------------------------------------------------------
// Return number of created serial port links
//
kern_return_t VSPDriver::getPortLinkCount(uint8_t* count)
{
    if (count == nullptr) {
        VSPLog(LOG_PREFIX, "getPortLinkCount: Invalid argument.");
        return kIOReturnBadArgument;
    }

    (*count) = ivars->m_portLinkCount;

    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// Return a serial port link item specified by id
//
kern_return_t VSPDriver::getPortLinkById(uint8_t id, void** result)
{
    if (!checkPortLinkId(id) || !result) {
        VSPLog(LOG_PREFIX, "getPortLinkById: Invalid argument. linkId=%d", id);
        return kIOReturnBadArgument;
    }
    
    if (ivars->m_portLinkCount == 0 || !ivars->m_portLinks) {
        VSPLog(LOG_PREFIX, "removePortLink: No serial port links available.");
        return kIOReturnNotFound;
    }

    for (uint8_t i = 0; i < ivars->m_portLinkCount; i++) {
        if (ivars->m_portLinks[i]->id == id) {
            (*result) = ivars->m_portLinks[i];
            return kIOReturnSuccess;
        }
    }

    return kIOReturnNotFound;
}

// --------------------------------------------------------------------
// Return a serial port link item specified by source and target ID of
// a serial port instance
//
kern_return_t VSPDriver::getPortLinkByPorts(uint8_t sourceId, uint8_t targetId, void** link)
{
    if (!checkPortId(sourceId) || !checkPortId(targetId) || !link) {
        VSPLog(LOG_PREFIX, "getPortLinkByPorts: Invalid argument. srcId=%d tgtId=%d",
               sourceId, targetId);
        return kIOReturnBadArgument;
    }
    
    uint8_t count = ivars->m_portLinkCount; 
    if (count && ivars->m_portLinks) {
        for (uint8_t i = 0; i < count; i++) {
            // skip invalid entry
            if (!ivars->m_portLinks[i]) {
                continue;;
            }
            TVSPLinkItem* item = ivars->m_portLinks[i];
            if (item->sourcePort.id == sourceId && item->targetPort.id == targetId) {
                (*link) = item;
                return kIOReturnSuccess;
            }
        }
    }

    return kIOReturnNotFound;
}
