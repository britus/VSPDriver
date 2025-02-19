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

using namespace VSPController;

#define LOG_PREFIX "VSPDriver"

#define kVSPSerialPortProperties "SerialPortProperties"
#define kVSPContollerProperties  "UserClientProperties"

// Driver instance state resource
struct VSPDriver_IVars {
    uint8_t        m_portCount     = 0;              // number of allocated serial ports
    TVSPPortItem*   m_serialPorts   = nullptr;      // list of allocated serial ports
    uint8_t        m_portLinkCount = 0;          // number of serial port links
    TVSPLinkItem*  m_portLinks     = nullptr;        // list of serial port links
};

// --------------------------------------------------------------------
// Allocate internal resources
//
bool VSPDriver::init(void)
{
    bool result;
    
    VSPLog(LOG_PREFIX, "init called.\n");
    
    if (!(result = super::init())) {
        VSPErr(LOG_PREFIX, "free (super) falsed. result=%d\n", result);
        goto finish;
    }
    
    // Create instance state resource
    ivars = IONewZero(VSPDriver_IVars, 1);
    if (!ivars) {
        VSPErr(LOG_PREFIX, "Unable to allocate driver data.\n");
        result = false;
        goto finish;
    }

    ivars->m_serialPorts = IONewZero(TVSPPortItem, MAX_SERIAL_PORTS);
    if (!ivars->m_serialPorts) {
        VSPErr(LOG_PREFIX, "CreateSerialPort: Out of memory.\n");
        return false;
    }

    ivars->m_portLinks = IONewZero(TVSPLinkItem, MAX_PORT_LINKS);
    if (!ivars->m_portLinks) {
        VSPErr(LOG_PREFIX, "CreateSerialPort: Out of memory.\n");
        return false;
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

    // release all link items and the list
    IOSafeDeleteNULL(ivars->m_portLinks, TVSPLinkItem, MAX_PORT_LINKS);
    
    // release all port items and the list
    IOSafeDeleteNULL(ivars->m_serialPorts, VSPSerialPort, MAX_SERIAL_PORTS);

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
        VSPErr(LOG_PREFIX, "Start: Private driver instance is NULL\n");
        return kIOReturnInvalid;
    }
    
    // Start service instance (Apple style super call)
    ret = Start(provider, SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "Start(super): failed. code=%d\n", ret);
        return ret;
    }
    
    // Create 4 serial port instances with each IOSerialBSDClient as a child instance
    if ((ret = CreateSerialPort(provider, 4)) != kIOReturnSuccess) {
        goto finish;
    }
    
    // Register driver instance to IOReg
    if ((ret = RegisterService()) != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "Start: RegisterService failed. code=%d\n", ret);
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
  
    // shutdown serial port instances
    for(uint8_t i = 0; i < MAX_SERIAL_PORTS ; i++) {
        if (ivars->m_serialPorts[i].id) {
            removePort(ivars->m_serialPorts[i].id);
        }
    }

    // service instance (Apple style super call)
    if ((ret= Stop(provider, SUPERDISPATCH)) != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "Stop (suprt) failed. code=%d\n", ret);
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
        VSPErr(LOG_PREFIX, "NewUserClient: Invalid argument.\n");
        return kIOReturnBadArgument;
    }
    
    if ((ret = CreateUserClient(GetProvider(), userClient)) != kIOReturnSuccess) {
        return ret;
    }
    
    VSPLog(LOG_PREFIX, "NewUserClient finished.\n");
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
        VSPErr(LOG_PREFIX, "CreateUserClient: Bad argument 'userClient' detected.\n");
        return kIOReturnBadArgument;
    }
    
    // Create sub service object from UserClientProperties in Info.plist
    ret= Create(this, kVSPContollerProperties, &service);
    if (ret != kIOReturnSuccess || service == nullptr) {
        VSPErr(LOG_PREFIX, "CreateUserClient: create failed. code=%d\n", ret);
        return ret;
    }
    
    VSPLog(LOG_PREFIX, "CreateUserClient: check VSPUserClient type.\n");
    
    // Sane check object type
    VSPUserClient* client;
    client = OSDynamicCast(VSPUserClient, service);
    if (client == nullptr) {
        VSPErr(LOG_PREFIX, "CreateUserClient: Cast to VSPUserClient failed.\n");
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
    
    (*count) = 0;

    for(uint8_t i = 0, j = 0; i < MAX_SERIAL_PORTS; i++) {
        if (ivars->m_serialPorts[i].id && ivars->m_serialPorts[i].port) {
            (*count) = ++j;
        }
    }

    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// Return number of created serial port links
//
kern_return_t VSPDriver::getPortLinkCount(uint8_t* count)
{
    if (count == nullptr) {
        VSPErr(LOG_PREFIX, "getPortLinkCount: Invalid argument.");
        return kIOReturnBadArgument;
    }
    
    (*count) = 0;
    
    for(uint8_t i = 0, j = 0; i < MAX_PORT_LINKS; i++) {
        if (ivars->m_portLinks[i].id) {
            (*count) = ++j;
        }
    }

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
    
    if (ivars->m_portCount == 0) {
        return kIOReturnNotFound;
    }
    
    for(uint8_t i = 0, j = 0; i < MAX_SERIAL_PORTS && j < count; i++) {
        if (ivars->m_serialPorts[i].id && ivars->m_serialPorts[i].port) {
            list[j++] = ivars->m_serialPorts[i].id;
        }
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
    
    if (ivars->m_portLinkCount == 0) {
        return kIOReturnNotFound;
    }
    
    for(uint8_t i = 0, j = 0; i < MAX_PORT_LINKS && j < count; i++) {
        if (ivars->m_portLinks[i].id) {
            const uint8_t lid = ivars->m_portLinks[i].id;
            const uint8_t src = ivars->m_portLinks[i].sourcePort.id;
            const uint8_t tgt = ivars->m_portLinks[i].targetPort.id;
            list[j++] = (((lid << 16) | (src << 8) | tgt) & 0x00ffffffL);
        }
    }
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// Return a serial port link item specified by id
//
kern_return_t VSPDriver::getPortById(uint8_t id, void* result, const uint32_t size)
{
    if (!checkPortId(id) || !result || size < sizeof(TVSPPortItem)) {
        VSPErr(LOG_PREFIX, "getPortLinkById: Invalid argument. linkId=%d", id);
        return kIOReturnBadArgument;
    }
    if (size < sizeof(TVSPPortItem)) {
        VSPErr(LOG_PREFIX, "removePortLink: Invalid structure size parameter.");
        return kIOReturnBadArgument;
    }

    if (ivars->m_portCount) {
        for (uint8_t i = 0; i < MAX_PORT_LINKS; i++) {
            if (ivars->m_serialPorts[i].id == id) {
                memcpy(result, &ivars->m_serialPorts[i], sizeof(TVSPPortItem));
                return kIOReturnSuccess;
            }
        }
    }

    return kIOReturnNotFound;
}
// --------------------------------------------------------------------
// Return a serial port link item specified by id
//
kern_return_t VSPDriver::getPortLinkById(uint8_t id, void* result, const uint32_t size)
{
    if (!checkPortLinkId(id) || !result || size < sizeof(TVSPLinkItem)) {
        VSPErr(LOG_PREFIX, "getPortLinkById: Invalid argument. linkId=%d", id);
        return kIOReturnBadArgument;
    }
    if (size < sizeof(TVSPLinkItem)) {
        VSPErr(LOG_PREFIX, "removePortLink: Invalid structure size parameter.");
        return kIOReturnBadArgument;
    }

    if (ivars->m_portLinkCount) {
        for (uint8_t i = 0; i < MAX_PORT_LINKS; i++) {
            if (ivars->m_portLinks[i].id == id) {
                memcpy(result, &ivars->m_portLinks[i], sizeof(TVSPLinkItem));
                return kIOReturnSuccess;
            }
        }
    }

    return kIOReturnNotFound;
}

// --------------------------------------------------------------------
// Return a serial port link item specified by source and target ID of
// a serial port instance
//
kern_return_t VSPDriver::getPortLinkByPorts(uint8_t sourceId, uint8_t targetId, void* link, const uint32_t size)
{
    if (!checkPortId(sourceId) || !checkPortId(targetId) || !link || size < sizeof(TVSPLinkItem)) {
        VSPErr(LOG_PREFIX, "getPortLinkByPorts: Invalid argument. srcId=%d tgtId=%d",
               sourceId, targetId);
        return kIOReturnBadArgument;
    }
       
    if (ivars->m_portLinkCount && ivars->m_portLinks) {
        for (uint8_t i = 0; i < MAX_PORT_LINKS; i++) {
            if (ivars->m_portLinks[i].id &&
                ivars->m_portLinks[i].sourcePort.id == sourceId && //
                ivars->m_portLinks[i].targetPort.id == targetId) {
                memcpy(link, &ivars->m_portLinks[i], sizeof(TVSPLinkItem));
                return kIOReturnSuccess;
            }
        }
    }

    return kIOReturnNotFound;
}

// --------------------------------------------------------------------
// Create given number of VSPSerialPort instances
//
kern_return_t VSPDriver::CreateSerialPort(IOService* provider, uint8_t count, void* params, uint64_t size)
{
    kern_return_t ret;
    IOService* service;
    
    VSPLog(LOG_PREFIX, "CreateSerialPort: create %d x VSPSerialPort from Info.plist.\n", count);

    if ((ivars->m_portCount + count) >= MAX_SERIAL_PORTS) {
        VSPErr(LOG_PREFIX, "createPort: Maximum of %d serial ports reached.\n",
               ivars->m_portCount);
        return kIOReturnNoSpace;
    }

    // do it
    for (uint8_t i = 0, items = 0, _id = 1; i < MAX_SERIAL_PORTS && items < count; i++, _id++) {
        
        /* skip entries that already in use */
        if (ivars->m_serialPorts[i].id || ivars->m_serialPorts[i].port) {
            continue;
        }

        VSPLog(LOG_PREFIX, "CreateSerialPort: Create serial port %d.\n", _id);

        // Create sub service object from SerialPortProperties in Info.plist
        ret= Create(this, kVSPSerialPortProperties, &service);
        if (ret != kIOReturnSuccess || service == nullptr) {
            VSPErr(LOG_PREFIX, "CreateSerialPort: create [%d] failed. code=%d\n", count, ret);
            return ret;
        }
        
        VSPLog(LOG_PREFIX, "CreateSerialPort: check VSPSerialPort type.\n");
        
        // Sane check object type
        ivars->m_serialPorts[i].port = OSDynamicCast(VSPSerialPort, service);
        if (ivars->m_serialPorts[i].port == nullptr) {
            VSPErr(LOG_PREFIX, "CreateSerialPort: Cast to VSPSerialPort failed.\n");
            service->release();
            return kIOReturnInvalid;
        }

        // save instance for controller
        ivars->m_serialPorts[i].id = _id;
        ivars->m_serialPorts[i].port->setParent(this);           // set this as parent
        ivars->m_serialPorts[i].port->setPortIdentifier(_id);    // set unique identifier
        ivars->m_serialPorts[i].flags = 0x00;
        if (params && size == sizeof(TVSPPortParameters)) {
            TVSPPortParameters* p = reinterpret_cast<TVSPPortParameters*>(params);
            ivars->m_serialPorts[i].port->HwProgramUART( //
                            p->baudRate, //
                            p->dataBits, //
                            p->stopBits, //
                            p->flowCtrl);
        }
        ivars->m_portCount++;
        items++;
    }

    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// Create new VSPSerialPort incstance (called by VSPUserClient)
//
kern_return_t VSPDriver::createPort(void* params, uint64_t size)
{
    return CreateSerialPort(GetProvider(), 1, params, size);
}

// --------------------------------------------------------------------
// Remove specified VSPSerialPort incstance (called by VSPUserClient)
//
kern_return_t VSPDriver::removePort(uint8_t portId)
{
    IOReturn ret;

    if (ivars->m_portCount == 0) {
        return kIOReturnNotFound;
    }

    if (!checkPortId(portId)) {
        return kIOReturnBadArgument;
    }

    /* Find serial port in link table and remove the link */
    if (ivars->m_portLinkCount) {
        for (uint8_t i = 0; i < MAX_PORT_LINKS; i++) {
            if (ivars->m_portLinks[i].id &&                        //
                 (ivars->m_portLinks[i].sourcePort.id == portId || //
                  ivars->m_portLinks[i].targetPort.id == portId) )
            {
                removePortLink(ivars->m_portLinks[i].id);
            }
        }
    }
    
    // Find given port in port list
    for (uint8_t i = 0; i < MAX_SERIAL_PORTS; i++)
    {
        if (ivars->m_serialPorts[i].id == portId) {
            VSPLog(LOG_PREFIX, "removePort: Shutdown serial port #%d\n",
                   ivars->m_serialPorts[i].id);
            
            // Start an IOService termination.
            if (ivars->m_serialPorts[i].port) {
                ret = ivars->m_serialPorts[i].port->Terminate(0);
                if (ret != kIOReturnSuccess) {
                    VSPErr(LOG_PREFIX, "removePort: Shutdown serial port failed. code=%d\n", ret);
                    return ret;
                }
            }
            
            // done!
            ivars->m_portCount--;
            memset(&ivars->m_serialPorts[i], 0, sizeof(TVSPPortItem));
            return kIOReturnSuccess;
        }
    }
    
    return kIOReturnNotFound;
}

// --------------------------------------------------------------------
// Create a link between two serial ports
//
kern_return_t VSPDriver::portsInLinkList(uint8_t sourceId, uint8_t targetId)
{
    // Check port id's in existing links
    for (uint8_t i = 0; i < MAX_PORT_LINKS; i++) {
        
        // skip empty entries
        if (!ivars->m_portLinks[i].id) {
            continue;
        }
        
        if (ivars->m_portLinks[i].sourcePort.id == sourceId) {
            VSPErr(LOG_PREFIX, "createPortLink: Source port %d already assigned to link %d\n",
                   sourceId, ivars->m_portLinks[i].id);
            return kIOReturnBusy;
        }
        
        if (ivars->m_portLinks[i].targetPort.id == targetId) {
            VSPErr(LOG_PREFIX, "createPortLink: Target port %d already assigned to link %d\n",
                   targetId, ivars->m_portLinks[i].id);
            return kIOReturnBusy;
        }
    }
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// Create a link between two serial ports
//
kern_return_t VSPDriver::portsAssigned(uint8_t sourceId, uint8_t targetId)
{
    uint8_t id;
    
    // check serial port list
    for (uint8_t i = 0; i < MAX_SERIAL_PORTS; i++) {
        
        // skip empty entries
        if (!ivars->m_serialPorts[i].id || !ivars->m_serialPorts[i].port) {
            continue;
        }
        
        // check port assignment source and get port entry
        if (ivars->m_serialPorts[i].id == sourceId) {
            if ((id = ivars->m_serialPorts[i].port->getPortLinkIdentifier())) {
                VSPErr(LOG_PREFIX, "createPortLink: Source port %d already assigned to link %d.",
                       ivars->m_serialPorts[i].id, id);
                return kIOReturnBusy;
            }
        }

        // check port assignment target and get port entry
        if (ivars->m_serialPorts[i].id == targetId) {
            if ((id = ivars->m_serialPorts[i].port->getPortLinkIdentifier())) {
                VSPErr(LOG_PREFIX, "createPortLink: Target port %d already assigned to link %d.",
                       ivars->m_serialPorts[i].id, id);
                return kIOReturnBusy;
            }
        }
    }

    return kIOReturnSuccess;
}

kern_return_t VSPDriver::createPortLink(uint8_t sourceId, uint8_t targetId, void* link, const uint32_t size)
{
    IOReturn ret;
    
    if (!link || !checkPortId(sourceId) || !checkPortId(targetId) || size < sizeof(TVSPLinkItem)) {
        VSPErr(LOG_PREFIX, "createPortLink: Invalid arguments. srcId=%d tgtId=%d size=%d\n",
               sourceId, targetId, size);
        return kIOReturnBadArgument;
    }
    
    if (ivars->m_portLinkCount >= MAX_PORT_LINKS) {
        VSPErr(LOG_PREFIX, "createPortLink: Maximum of %d links reached. srcId=%d tgtId=%d\n",
               ivars->m_portLinkCount, sourceId, targetId);
        return kIOReturnNoSpace;
    }

    if (sourceId == targetId) {
        VSPErr(LOG_PREFIX, "createPortLink: Link of same ports prohibited. srcId=%d tgtId=%d\n",
               sourceId, targetId);
        return kIOReturnInvalid;
    }
    
    if ((ret = portsInLinkList(sourceId, targetId)) != kIOReturnSuccess) {
        return ret;
    }
    
    if ((ret = portsAssigned(sourceId, targetId)) != kIOReturnSuccess) {
        return ret;
    }

    TVSPPortItem src = {};
    if ((ret = getPortById(sourceId, &src, sizeof(TVSPPortItem))) != kIOReturnSuccess) {
        return ret;
    }

    TVSPPortItem tgt = {};
    if ((ret = getPortById(targetId, &tgt, sizeof(TVSPPortItem))) != kIOReturnSuccess) {
        return ret;
    }

    if (!src.port || !tgt.port) {
        VSPErr(LOG_PREFIX, "createPortLink: Invalid port in sourceId=%d or targetId=%d\n",
               sourceId, targetId);
        return kIOReturnNotFound;
    }
    
    // Create new port link
    for (uint8_t i = 0, _id = 1; i < MAX_PORT_LINKS; i++, _id++) {
     
        // skip entries already in use
        if (ivars->m_portLinks[i].id) {
            continue;
        }

        // setup new port link item
        ivars->m_portLinks[i].id = _id;
        ivars->m_portLinks[i].sourcePort.id = src.id;
        ivars->m_portLinks[i].sourcePort.port = src.port;
        ivars->m_portLinks[i].sourcePort.flags = 0x01;
        ivars->m_portLinks[i].targetPort.id = tgt.id;
        ivars->m_portLinks[i].targetPort.port = tgt.port;
        ivars->m_portLinks[i].targetPort.flags = 0x01;
        
        // return link
        memcpy(link, &ivars->m_portLinks[i], sizeof(TVSPLinkItem));
        
        // assign new link
        src.port->setPortLinkIdentifier(_id);
        tgt.port->setPortLinkIdentifier(_id);
        
        ivars->m_portLinkCount++;
        break; // done!
    }
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// Removes prior created serial port link
//
kern_return_t VSPDriver::removePortLink(uint8_t linkId)
{
    if (!linkId) {
        VSPErr(LOG_PREFIX, "removePortLink: Invalid argument.");
        return kIOReturnBadArgument;
    }
    
    if (ivars->m_portLinkCount == 0) {
        VSPErr(LOG_PREFIX, "removePortLink: No serial port links available.");
        return kIOReturnNotFound;
    }
            
    /* find given link item and exclude this from new new list */
    for (uint8_t i = 0; i < MAX_PORT_LINKS; i++) {
        if (ivars->m_portLinks[i].id == linkId) {
            VSPLog(LOG_PREFIX, "removePortLink: Link Id=%d found :)\n", linkId);
            if (ivars->m_portLinks[i].sourcePort.port) {
                ivars->m_portLinks[i].sourcePort.port->setPortLinkIdentifier(0);
            }
            if (ivars->m_portLinks[i].targetPort.port) {
                ivars->m_portLinks[i].targetPort.port->setPortLinkIdentifier(0);
            }
            memset(&ivars->m_portLinks[i], 0, sizeof(TVSPLinkItem));
            ivars->m_portLinkCount--;
            return kIOReturnSuccess; // done!
        }
    }

    return kIOReturnNotFound;
}
