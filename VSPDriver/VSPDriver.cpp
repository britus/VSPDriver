// ********************************************************************
// VSPDriver - Driver root object
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************

// -- OS
#include <stdio.h>
#include <os/log.h>
#include <time.h>
#include <math.h>

#include <DriverKit/OSBundle.h>
#include <DriverKit/OSString.h>
#include <DriverKit/OSNumber.h>
#include <DriverKit/OSBoolean.h>
#include <DriverKit/OSArray.h>
#include <DriverKit/OSSet.h>
#include <DriverKit/OSOrderedSet.h>
#include <DriverKit/OSPtr.h>
#include <DriverKit/OSData.h>
#include <DriverKit/OSDictionary.h>
#include <DriverKit/OSCollection.h>
#include <DriverKit/OSCollections.h>
#include <DriverKit/IOLib.h>
#include <DriverKit/IOTypes.h>
#include <DriverKit/IOService.h>
#include <DriverKit/IOMemoryMap.h>
#include <DriverKit/IOMemoryDescriptor.h>
#include <DriverKit/IOBufferMemoryDescriptor.h>
#include <DriverKit/IODispatchQueue.h>
#include <DriverKit/IODispatchSource.h>

#include <DriverKit/IODataQueueDispatchSource.h>
#include <DriverKit/IOInterruptDispatchSource.h>
#include <DriverKit/IOTimerDispatchSource.h>
#include <DriverKit/IOServiceNotificationDispatchSource.h>
#include <DriverKit/IOServiceStateNotificationDispatchSource.h>

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
    uint8_t m_portCount = 0;                // number of allocated serial ports
    TVSPPortItem* m_ports = nullptr;        // list of allocated serial ports
    uint8_t m_linkCount = 0;                // number of serial port links
    TVSPLinkItem* m_links = nullptr;        // list of serial port links
    IODispatchQueue* m_serviceQueue = nullptr; // default dispatch queue
    uint64_t m_traceFlags;
    uint64_t m_checkFlags;
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
    
    ivars->m_ports = IONewZero(TVSPPortItem, MAX_SERIAL_PORTS);
    if (!ivars->m_ports) {
        VSPErr(LOG_PREFIX, "CreateSerialPort: Out of memory.\n");
        return false;
    }
    
    ivars->m_links = IONewZero(TVSPLinkItem, MAX_PORT_LINKS);
    if (!ivars->m_links) {
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
    IOSafeDeleteNULL(ivars->m_links, TVSPLinkItem, MAX_PORT_LINKS);
    
    // release all port items and the list
    IOSafeDeleteNULL(ivars->m_ports, VSPSerialPort, MAX_SERIAL_PORTS);
    
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
        VSPErr(LOG_PREFIX, "Start(super): failed. code=%x\n", ret);
        return ret;
    }
    
    ret = CopyDispatchQueue(kIOServiceDefaultQueueName, &ivars->m_serviceQueue);
    if (ret != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "Start: Copy dispatch queue failed. code=%x\n", ret);
        return ret;
    }

    // Create 4 serial port instances with each IOSerialBSDClient as a child instance
    if ((ret = CreateSerialPort(provider, 4)) != kIOReturnSuccess) {
        goto finish;
    }
    
    // Register driver instance to IOReg
    if ((ret = RegisterService()) != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "Start: RegisterService failed. code=%x\n", ret);
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
        if (ivars->m_ports[i].id) {
            removePort(ivars->m_ports[i].id);
        }
    }
    
    OSSafeReleaseNULL(ivars->m_serviceQueue);

    // service instance (Apple style super call)
    if ((ret= Stop(provider, SUPERDISPATCH)) != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "Stop (suprt) failed. code=%x\n", ret);
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
    
    // Create sub service object from Info.plist
    ret= Create(this, kVSPContollerProperties, &service);
    if (ret != kIOReturnSuccess || service == nullptr) {
        VSPErr(LOG_PREFIX, "CreateUserClient: create failed. code=%x\n", ret);
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
        if (ivars->m_ports[i].id && ivars->m_ports[i].port) {
            (*count) = ++j;
        }
    }
    
    return kIOReturnSuccess;
}

kern_return_t VSPDriver::getNextPortId(uint8_t* id)
{
    uint8_t _id = 0;
    
    if (id == nullptr) {
        return kIOReturnBadArgument;
    }
    
    for(uint8_t i = 0; i < MAX_SERIAL_PORTS; i++) {
        if (ivars->m_ports[i].id && ivars->m_ports[i].port) {
            if ((_id + 1) == ivars->m_ports[i].id) {
                _id = ivars->m_ports[i].id;
            }
        }
    }
    
    (*id) = (_id + 1);
    
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
        if (ivars->m_links[i].id) {
            (*count) = ++j;
        }
    }
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// Return all IDs of all allocated serial ports
//
kern_return_t VSPDriver::getPortList(void* list, uint8_t count)
{
    TVSPPortListItem* items = (TVSPPortListItem*) list;
    VSPSerialPort* port;
    IOPropertyName name;
    
    if (!list) {
        return kIOReturnBadArgument;
    }
    
    if (ivars->m_portCount == 0) {
        return kIOReturnNotFound;
    }
    
    for(uint8_t i = 0, j = 0; i < MAX_SERIAL_PORTS && j < count; i++) {
        if (ivars->m_ports[i].id && (port = ivars->m_ports[i].port)) {
            items[j].id = ivars->m_ports[i].id;
            if (port->getDeviceName(name, sizeof(IOPropertyName)) != kIOReturnSuccess) {
                continue;
            }
            items[j].id = ivars->m_ports[i].id;
            items[j].flags = (port->traceFlags() | port->checkFlags());
            strncpy(items[j].name, name, MAX_PORT_NAME);
            j++;
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
    
    if (ivars->m_linkCount == 0) {
        return kIOReturnNotFound;
    }
    
    for(uint8_t i = 0, j = 0; i < MAX_PORT_LINKS && j < count; i++) {
        if (ivars->m_links[i].id) {
            const uint8_t lid = ivars->m_links[i].id;
            const uint8_t src = ivars->m_links[i].source.id;
            const uint8_t tgt = ivars->m_links[i].target.id;
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
        VSPErr(LOG_PREFIX, "getPortById: Invalid argument. portId=%d size=%d", id, size);
        return kIOReturnBadArgument;
    }
    
    if (ivars->m_portCount) {
        for (uint8_t i = 0; i < MAX_PORT_LINKS; i++) {
            if (ivars->m_ports[i].id == id) {
                memcpy(result, &ivars->m_ports[i], sizeof(TVSPPortItem));
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
        VSPErr(LOG_PREFIX, "getPortLinkById: Invalid argument. linkId=%d size=%d", id, size);
        return kIOReturnBadArgument;
    }
    
    if (ivars->m_linkCount) {
        for (uint8_t i = 0; i < MAX_PORT_LINKS; i++) {
            if (ivars->m_links[i].id == id) {
                memcpy(result, &ivars->m_links[i], sizeof(TVSPLinkItem));
                return kIOReturnSuccess;
            }
        }
    }
    
    return kIOReturnNotFound;
}

kern_return_t VSPDriver::getPortDeviceName(uint8_t id, char* result, const uint32_t size)
{
    if (!checkPortId(id) || !result || size < sizeof(IOPropertyName)) {
        VSPErr(LOG_PREFIX, "getPortDeviceName: Invalid argument. portId=%d size=%d", id, size);
        return kIOReturnBadArgument;
    }
    
    if (ivars->m_portCount) {
        for (uint8_t i = 0; i < MAX_PORT_LINKS; i++) {
            if (ivars->m_ports[i].id == id && ivars->m_ports[i].port) {
                return ivars->m_ports[i].port->getDeviceName(result, size);
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
    
    if (ivars->m_linkCount && ivars->m_links) {
        for (uint8_t i = 0; i < MAX_PORT_LINKS; i++) {
            if (ivars->m_links[i].id &&
                ivars->m_links[i].source.id == sourceId && //
                ivars->m_links[i].target.id == targetId) {
                memcpy(link, &ivars->m_links[i], sizeof(TVSPLinkItem));
                return kIOReturnSuccess;
            }
        }
    }
    
    return kIOReturnNotFound;
}

// --------------------------------------------------------------------
// SPReadyEvent(OSAction* action)
// Called by service ready notification event
//
void IMPL(VSPDriver, SPReadyEvent)
{
    VSPLog(LOG_PREFIX, "SPReadyEvent: called.\n");

    if (!action) {
        VSPErr(LOG_PREFIX, "SPReadyEvent: Invalid action argument.\n");
        return;
    }
 
    VSPLog(LOG_PREFIX, "SPReadyEvent: complete\n");
}

// --------------------------------------------------------------------
// SPStateEvent(OSAction* action)
// Called by service state notification event
//
void IMPL(VSPDriver, SPStateEvent)
{
    VSPLog(LOG_PREFIX, "SPStateEvent: called.\n");

    if (!action) {
        VSPErr(LOG_PREFIX, "SPStateEvent: Invalid action argument.\n");
        return;
    }

    VSPLog(LOG_PREFIX, "SPStateEvent: complete\n");
}

// --------------------------------------------------------------------
// Create given number of VSPSerialPort instances
//
kern_return_t VSPDriver::CreateSerialPort(IOService* provider, uint8_t count, void* params, uint64_t flags, uint64_t size)
{
    //const uint64_t TRACE_MASK = (TRACE_PORT_RX|TRACE_PORT_TX|TRACE_PORT_IO);
    //const uint64_t CHECK_MASK = (CHECK_BAUD|CHECK_PARITY|CHECK_FLOWCTRL|CHECK_DATA_SIZE|CHECK_STOP_BITS);
    kern_return_t ret;
    IOService* service = nullptr;
        
    VSPLog(LOG_PREFIX, "CreateSerialPort: create %d x VSPSerialPort from Info.plist.\n", count);
    
    if ((ivars->m_portCount + count) > MAX_SERIAL_PORTS) {
        VSPErr(LOG_PREFIX, "CreateSerialPort: Maximum of %d of %d serial ports reached.\n",
               MAX_SERIAL_PORTS, ivars->m_portCount + count);
        return kIOReturnNoSpace;
    }
    
    for (uint8_t i = 0, items = 0; i < MAX_SERIAL_PORTS && items < count; i++) {
        /* skip entries that already in use */
        if (ivars->m_ports[i].id || ivars->m_ports[i].port) {
            continue;
        }
        
        VSPLog(LOG_PREFIX, "CreateSerialPort: Create serial port.\n");

        // reset entry first
        ivars->m_ports[i] = {};
        
        // Create sub service VSPSerialPort object from Info.plist
        ret = Create(this, kVSPSerialPortProperties, &service);
        if (ret != kIOReturnSuccess) {
            VSPErr(LOG_PREFIX, "CreateSerialPort: create [%d] failed. code=%x\n", count, ret);
            return ret;
        }
        
        // Sane object type check
        if ((ivars->m_ports[i].port = OSDynamicCast(VSPSerialPort, service)) == nullptr) {
            VSPErr(LOG_PREFIX, "CreateSerialPort: Cast to VSPSerialPort failed.\n");
            service->release();
            return kIOReturnInvalid;
        }

        // Set 'this' instance as parent and update port item
        ivars->m_ports[i].port->setPortItem(this, &ivars->m_ports[i]);
        
        // Setup user defined serial port parameters
        if (params && size == sizeof(TVSPPortParameters)) {
            TVSPPortParameters* pp = reinterpret_cast<TVSPPortParameters*>(params);
            if (pp->baudRate && pp->dataBits) {
                ivars->m_ports[i].port->HwProgramUART( //
                    pp->baudRate,   //
                    pp->dataBits,   //
                    pp->stopBits,   //
                    pp->flowCtrl);
            }
        }
        
        // next...
        ivars->m_portCount++;
        items++;
    }
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// Create new VSPSerialPort incstance (called by VSPUserClient)
//
kern_return_t VSPDriver::createPort(void* params, uint64_t flags, uint64_t size)
{
    return CreateSerialPort(GetProvider(), 1, params, flags, size);
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
    if (ivars->m_linkCount) {
        for (uint8_t i = 0; i < MAX_PORT_LINKS; i++) {
            if (ivars->m_links[i].id &&                        //
                (ivars->m_links[i].source.id == portId || //
                 ivars->m_links[i].target.id == portId) )
            {
                removePortLink(ivars->m_links[i].id);
            }
        }
    }
    
    // Find given port in port list
    for (uint8_t i = 0; i < MAX_SERIAL_PORTS; i++) {
        if (ivars->m_ports[i].id == portId) {
            VSPLog(LOG_PREFIX, "removePort: Shutdown serial port #%d\n",
                   ivars->m_ports[i].id);
            
            // Start an IOService termination.
            if (ivars->m_ports[i].port) {
                ret = ivars->m_ports[i].port->Terminate(0);
                if (ret != kIOReturnSuccess) {
                    VSPErr(LOG_PREFIX, "removePort: Shutdown serial port failed. code=%x\n", ret);
                    return ret;
                }
            }

            // done!
            ivars->m_portCount--;
            memset(&ivars->m_ports[i], 0, sizeof(TVSPPortItem));
            return kIOReturnSuccess;
        }
    }
    
    return kIOReturnNotFound;
}

// --------------------------------------------------------------------
// Check given port id's in link table. Returns kIOReturnBusy if found
//
kern_return_t VSPDriver::portsInLinkList(uint8_t sourceId, uint8_t targetId)
{
    // Check port id's in existing links
    for (uint8_t i = 0; i < MAX_PORT_LINKS; i++) {
        // skip empty entries
        if (!ivars->m_links[i].id) {
            continue;
        }
        
        if (ivars->m_links[i].source.id == sourceId) {
            VSPErr(LOG_PREFIX, "createPortLink: Source port %d already assigned to link %d\n",
                   sourceId, ivars->m_links[i].id);
            return kIOReturnBusy;
        }
        
        if (ivars->m_links[i].target.id == targetId) {
            VSPErr(LOG_PREFIX, "createPortLink: Target port %d already assigned to link %d\n",
                   targetId, ivars->m_links[i].id);
            return kIOReturnBusy;
        }
    }
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// Check given port id's already assigned to link item. Returns
// kIOReturnBusy if found
//
kern_return_t VSPDriver::portsAssigned(uint8_t sourceId, uint8_t targetId)
{
    uint8_t id;
    
    // check serial port list
    for (uint8_t i = 0; i < MAX_SERIAL_PORTS; i++) {
        // skip empty entries
        if (!ivars->m_ports[i].id || !ivars->m_ports[i].port) {
            continue;
        }
        
        // check port assignment source and get port entry
        if (ivars->m_ports[i].id == sourceId) {
            if ((id = ivars->m_ports[i].port->getPortLinkIdentifier())) {
                VSPErr(LOG_PREFIX, "createPortLink: Source port %d already assigned to link %d.",
                       ivars->m_ports[i].id, id);
                return kIOReturnBusy;
            }
        }
        
        // check port assignment target and get port entry
        if (ivars->m_ports[i].id == targetId) {
            if ((id = ivars->m_ports[i].port->getPortLinkIdentifier())) {
                VSPErr(LOG_PREFIX, "createPortLink: Target port %d already assigned to link %d.",
                       ivars->m_ports[i].id, id);
                return kIOReturnBusy;
            }
        }
    }
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// Create a serial port link for given port id's.
//
kern_return_t VSPDriver::createPortLink(uint8_t sourceId, uint8_t targetId, void* link, const uint32_t size)
{
    IOReturn ret;
    
    if (!link || !checkPortId(sourceId) || !checkPortId(targetId) || size < sizeof(TVSPLinkItem)) {
        VSPErr(LOG_PREFIX, "createPortLink: Invalid arguments. srcId=%d tgtId=%d size=%d\n",
               sourceId, targetId, size);
        return kIOReturnBadArgument;
    }
    
    if (ivars->m_linkCount >= MAX_PORT_LINKS) {
        VSPErr(LOG_PREFIX, "createPortLink: Maximum of %d links reached. srcId=%d tgtId=%d\n",
               ivars->m_linkCount, sourceId, targetId);
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
        if (ivars->m_links[i].id) {
            continue;
        }
        
        // setup new port link item
        ivars->m_links[i].id = _id;
        ivars->m_links[i].source.id = src.id;
        ivars->m_links[i].source.port = src.port;
        ivars->m_links[i].source.flags = 0x01;
        ivars->m_links[i].target.id = tgt.id;
        ivars->m_links[i].target.port = tgt.port;
        ivars->m_links[i].target.flags = 0x01;
        
        // return link
        memcpy(link, &ivars->m_links[i], sizeof(TVSPLinkItem));
        
        // assign new link
        src.port->setPortLinkIdentifier(_id);
        tgt.port->setPortLinkIdentifier(_id);
        
        ivars->m_linkCount++;
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
    
    if (ivars->m_linkCount == 0) {
        VSPErr(LOG_PREFIX, "removePortLink: No serial port links available.");
        return kIOReturnNotFound;
    }
    
    /* find given link item and exclude this from new new list */
    for (uint8_t i = 0; i < MAX_PORT_LINKS; i++) {
        if (ivars->m_links[i].id == linkId) {
            VSPLog(LOG_PREFIX, "removePortLink: Link Id=%d found :)\n", linkId);
            if (ivars->m_links[i].source.port) {
                ivars->m_links[i].source.port->setPortLinkIdentifier(0);
            }
            if (ivars->m_links[i].target.port) {
                ivars->m_links[i].target.port->setPortLinkIdentifier(0);
            }
            memset(&ivars->m_links[i], 0, sizeof(TVSPLinkItem));
            ivars->m_linkCount--;
            return kIOReturnSuccess; // done!
        }
    }
    
    return kIOReturnNotFound;
}

uint64_t VSPDriver::traceFlags()
{
    return ivars->m_traceFlags;
}

void VSPDriver::setTraceFlags(uint64_t flags)
{
    ivars->m_traceFlags = flags;
}

uint64_t VSPDriver::checkFlags()
{
    return ivars->m_checkFlags;
}

void VSPDriver::setCheckFlags(uint64_t flags)
{
    ivars->m_checkFlags = flags;
}
