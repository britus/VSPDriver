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
#include "VSPSerialPort.h"
#include "VSPController.h"
#include "VSPUserClient.h"

#define LOG_PREFIX "VSPDriver"

#define kVSPSerialPortProperties "SerialPortProperties"
#define kVSPContollerProperties  "UserClientProperties"
#define kVSPDefaultPortCount 4
#define kVSPMaxumumPorts 16

// Driver instance state resource
struct VSPDriver_IVars {
    uint8_t               m_portCount = 0;              // number of allocated serial ports
    TVSPSerialPortItem**  m_serialPorts = nullptr;      // list of allocated serial ports
    uint8_t               m_portLinkCount = 0;          // number of serial port links
    TVSPPortLinkItem**    m_portLinks = nullptr;        // list of serial port links
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
        if (ivars->m_serialPorts && ivars->m_portCount) {
            for (uint8_t i = 0; i < ivars->m_portCount; i++) {
                ivars->m_serialPorts[i]->port->unlinkParent();
                IOSafeDeleteNULL(ivars->m_serialPorts[i], TVSPSerialPortItem, 1);
            }
        }
        if (ivars->m_portLinks && ivars->m_portLinkCount) {
            for (uint8_t i = 0; i < ivars->m_portLinkCount; i++) {
                IOSafeDeleteNULL(ivars->m_portLinks[i], TVSPPortLinkItem, 1);
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
    
    VSPLog(LOG_PREFIX, "CreateSerialPort: create #%d VSPSerialPort from Info.plist.\n", count);
    
    // reset first
    ivars->m_portCount = 0;

    // Allocate serial port list. Holds each allocated
    // instance of the VSPSerialPort object
    ivars->m_serialPorts = IONewZero(TVSPSerialPortItem*, count);
    if (!ivars->m_serialPorts) {
        VSPLog(LOG_PREFIX, "CreateSerialPort: Out of memory.\n");
        return kIOReturnNoMemory;
    }
    
    // do it
    for (uint8_t i = 0; i < count && i < kVSPMaxumumPorts; i++) {
        VSPLog(LOG_PREFIX, "CreateSerialPort: Create serial port %d.\n", i);
        
        TVSPSerialPortItem* item = IONewZero(TVSPSerialPortItem, 1);
        if (!item) {
            VSPLog(LOG_PREFIX, "CreateSerialPort: Ouch out of memory.\n");
            IOSafeDeleteNULL(ivars->m_serialPorts, TVSPSerialPortItem*, count);
            return kIOReturnNoMemory;
        }

        // Create sub service object from UserClientProperties in Info.plist
        ret= Create(this, kVSPSerialPortProperties, &service);
        if (ret != kIOReturnSuccess || service == nullptr) {
            VSPLog(LOG_PREFIX, "CreateSerialPort: create [%d] failed. code=%d\n", count, ret);
            IOSafeDeleteNULL(ivars->m_serialPorts, TVSPSerialPortItem*, count);
            IOSafeDeleteNULL(item, TVSPSerialPortItem, 1);
            return ret;
        }
        
        VSPLog(LOG_PREFIX, "CreateSerialPort: check VSPSerialPort type.\n");
        
        // Sane check object type
        item->port = OSDynamicCast(VSPSerialPort, service);
        if (item->port == nullptr) {
            VSPLog(LOG_PREFIX, "CreateSerialPort: Cast to VSPSerialPort failed.\n");
            IOSafeDeleteNULL(ivars->m_serialPorts, TVSPSerialPortItem*, count);
            IOSafeDeleteNULL(item, TVSPSerialPortItem, 1);
            service->release();
            return kIOReturnInvalid;
        }
        
        item->id = i + 1;
        item->flags = 0x00;
        item->port->setParent(this);                // set this as parent
        item->port->setPortIdentifier(item->id);    // set unique identifier
        
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

kern_return_t VSPDriver::getPortCount(uint8_t* count)
{
    if (!count) {
        return kIOReturnBadArgument;
    }
    
    (*count) = ivars->m_portCount;
    return kIOReturnSuccess;
}

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

kern_return_t VSPDriver::createPortLink(uint8_t sourceId, uint8_t targetId, void** link)
{
    if (!link || !sourceId || !targetId) {
        VSPLog(LOG_PREFIX, "createPortLink: Invalid arguments\n");
        return kIOReturnBadArgument;
    }
  
    TVSPSerialPortItem* src = nullptr;
    TVSPSerialPortItem* tgt = nullptr;
    
    // check serial port list
    for (uint8_t i = 0; i < ivars->m_portCount; i++) {
        
        // skip invalid entry
        if (!ivars->m_serialPorts[i] || !ivars->m_serialPorts[i]->port) {
            continue;
        }
        
        const TVSPSerialPortItem* port = ivars->m_serialPorts[i];
        uint8_t linkId;
        
        if (port->id == sourceId) {
            if ((linkId = port->port->getPortLinkIdentifier()) == 0) {
                src = ivars->m_serialPorts[i];
            } else {
                VSPLog(LOG_PREFIX, "createPortLink: Source port %d already assigned to link %d.",
                       port->id, linkId);
                return kIOReturnBadArgument;
            }
        }

        if (port->id == targetId) {
            if ((linkId = port->port->getPortLinkIdentifier()) == 0) {
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
            
            TVSPPortLinkItem* link = ivars->m_portLinks[i];
            
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
    TVSPPortLinkItem* item = nullptr;
    if (!(item = IONewZero(TVSPPortLinkItem, 1))) {
        VSPLog(LOG_PREFIX, "createPortLink: Out of memory!\n");
        return kIOReturnNoMemory;
    }
    
    // new list with additional space
    TVSPPortLinkItem** list = nullptr;
    if (!(list = IONewZero(TVSPPortLinkItem*, count + 1))) {
        VSPLog(LOG_PREFIX, "createPortLink: Out of memory!\n");
        return kIOReturnNoMemory;
    }
    
    /* copy existing links */
    if (count && ivars->m_portLinks) {
        for (uint8_t i = 0; i < count; i++) {
            if (ivars->m_portLinks[i] != nullptr) {
                list[i] = ivars->m_portLinks[i];
            }
        }
    }

    // old list to remove
    TVSPPortLinkItem** oldl = ivars->m_portLinks;

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
   
    // set new link item
    list[count] = item;
    
    // return new item
    (*link) = item;
    
    // update global members
    ivars->m_portLinks = list;
    ivars->m_portLinkCount++;

    IOSafeDeleteNULL(oldl, TVSPPortLinkItem*, count);
    return kIOReturnSuccess; // done!
}

kern_return_t VSPDriver::removePortLink(void *link)
{
    if (!link) {
        VSPLog(LOG_PREFIX, "removePortLink: Invalid argument.");
        return kIOReturnBadArgument;
    }
    
    if (ivars->m_portLinkCount == 0 || !ivars->m_portLinks) {
        VSPLog(LOG_PREFIX, "removePortLink: No serial port links available.");
        return kIOReturnSuccess;
    }
            
    TVSPPortLinkItem* item = reinterpret_cast<TVSPPortLinkItem*>(link);
    uint8_t count = ivars->m_portLinkCount;
    uint8_t found = 0;
    
    // old port list to remove
    TVSPPortLinkItem** oldl = ivars->m_portLinks;

    // new port link list
    TVSPPortLinkItem** list;
    if (!(list= IONewZero(TVSPPortLinkItem*, count - 1))) {
        VSPLog(LOG_PREFIX, "removePortLink: Out of memory.\n");
        return kIOReturnNoMemory;
    }

    /* find given link item and exclude this from new new list */
    for (uint8_t i = 0, j = 0; i < count; i++) {
        if (ivars->m_portLinks[i]->id == item->id) {
            if (item->sourcePort.port) {
                item->sourcePort.port->setPortLinkIdentifier(0);
            }
            if (item->targetPort.port) {
                item->targetPort.port->setPortLinkIdentifier(0);
            }
            found = 1;
        } else {
            list[j++] = ivars->m_portLinks[i];
        }
    }

    // remove port link
    if (found) {
        ivars->m_portLinks = list;
        ivars->m_portLinkCount--;

        IOSafeDeleteNULL(item, TVSPPortLinkItem, 1);
        IOSafeDeleteNULL(oldl, TVSPPortLinkItem*, count);
        return kIOReturnSuccess; // done!
    }

    IOSafeDeleteNULL(list, TVSPPortLinkItem*, count - 1);
    return kIOReturnNotFound;
}

kern_return_t VSPDriver::getPortLinkCount(uint8_t* count)
{
    if (!count) {
        VSPLog(LOG_PREFIX, "getPortLinkCount: Invalid argument.");
        return kIOReturnBadArgument;
    }
    
    (*count) = ivars->m_portLinkCount;
    return kIOReturnSuccess;
}

kern_return_t VSPDriver::getPortLinkById(uint8_t id, void** result)
{
    if (!id || !result) {
        VSPLog(LOG_PREFIX, "getPortLinkById: Invalid argument.");
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

kern_return_t VSPDriver::getPortLinkByPorts(uint8_t sourceId, uint8_t targetId, void** link)
{
    if (!sourceId || !targetId || !link) {
        VSPLog(LOG_PREFIX, "getPortLinkByPorts: Invalid argument.");
        return kIOReturnBadArgument;
    }
    
    uint8_t count = ivars->m_portLinkCount; 
    if (count && ivars->m_portLinks) {
        for (uint8_t i = 0; i < count; i++) {
            // skip invalid entry
            if (!ivars->m_portLinks[i]) {
                continue;;
            }
            TVSPPortLinkItem* item = ivars->m_portLinks[i];
            if (item->sourcePort.id == sourceId && item->targetPort.id == targetId) {
                (*link) = item;
                return kIOReturnSuccess;
            }
        }
    }

    return kIOReturnNotFound;
}
