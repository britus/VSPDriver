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

// Driver instance state resource
struct VSPDriver_IVars {
    VSPSerialPort** m_serialPorts;
    uint8_t         m_portCount;
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
        
    // release allocated port list only.
    // (not each instance itself!)
    if (ivars && ivars->m_serialPorts && ivars->m_portCount) {
        for (uint8_t i = 0; i < ivars->m_portCount; i++) {
            ivars->m_serialPorts[i]->unlinkParent();
            // OSSafeReleaseNULL(ivars->m_serialPorts[i]);
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
    VSPSerialPort* port;
    
    VSPLog(LOG_PREFIX, "CreateSerialPort: create #%d VSPSerialPort from Info.plist.\n", count);
    
    // Allocate serial port list. Holds each allocated
    // instance of the VSPSerialPort object
    ivars->m_serialPorts = IONewZero(VSPSerialPort*, count);
    if (!ivars->m_serialPorts) {
        VSPLog(LOG_PREFIX, "CreateSerialPort: Ouch out of memory.\n");
        return kIOReturnNoMemory;
    }
    
    // remember count for IOSafeDeleteNULL call
    ivars->m_portCount = count;
    
    // do it
    for (uint8_t i = 0; i < count; i++) {
        VSPLog(LOG_PREFIX, "CreateSerialPort: Create serial port %d.\n", i);
        
        // Create sub service object from UserClientProperties in Info.plist
        ret= Create(this, kVSPSerialPortProperties, &service);
        if (ret != kIOReturnSuccess || service == nullptr) {
            VSPLog(LOG_PREFIX, "CreateSerialPort: create [%d] failed. code=%d\n", count, ret);
            return ret;
        }
        
        VSPLog(LOG_PREFIX, "CreateSerialPort: check VSPSerialPort type.\n");
        
        // Sane check object type
        port = OSDynamicCast(VSPSerialPort, service);
        if (port == nullptr) {
            VSPLog(LOG_PREFIX, "CreateSerialPort: Cast to VSPSerialPort failed.\n");
            service->release();
            return kIOReturnInvalid;
        }
        
        // set this as parent
        port->setParent(this);
        
        // save instance for controller
        ivars->m_serialPorts[i] = port;
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
    (*userClient) = OSDynamicCast(VSPUserClient, service);
    if ((*userClient) == nullptr) {
        VSPLog(LOG_PREFIX, "CreateUserClient: Cast to VSPUserClient failed.\n");
        service->release();
        return kIOReturnInvalid;
    }
    
    VSPLog(LOG_PREFIX, "CreateUserClient: success.");
    return kIOReturnSuccess;
}
