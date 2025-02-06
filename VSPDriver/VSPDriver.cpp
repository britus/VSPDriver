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
#include "VSPLogger.h"
#include "VSPDriver.h"
#include "VSPSerialPort.h"

#define LOG_PREFIX "VSPDriver"

/* ============================================================
 <key>IOKitPersonalities</key>
 <dict>
     <key>VSPDriver</key>
     <dict>
         <key>CFBundleIdentifierKernel</key>
         <string>com.apple.kpi.iokit</string>
         <key>IOClass</key>
         <string>IOUserService</string>
         <key>IOMatchCategory</key>
         <string>org.eof.tools.VSPDriver</string>
         <key>IOProviderClass</key>
         <string>IOUserResources</string>
         <key>IOResourceMatch</key>
         <string>IOKit</string>
         <key>IOUserClass</key>
         <string>VSPDriver</string>
         <key>IOUserServerName</key>
         <string>org.eof.tools.VSPDriver</string>
         <key>SerialPortProperties</key>
         <dict>
             <key>CFBundleIdentifierKernel</key>
             <string>com.apple.driver.driverkit.serial</string>
             <key>IOProviderClass</key>
             <string>IOSerialStreamSync</string>
             <key>IOClass</key>
             <string>IOUserSerial</string>
             <key>IOUserClass</key>
             <string>VSPSerialPort</string>
             <key>HiddenPort</key>
             <false/>
             <key>IOTTYBaseName</key>
             <string>vsp</string>
             <key>IOTTYSuffix</key>
             <string>0</string>
         </dict>
     </dict>
 </dict>
 * ============================================================ */

#define kVSPSerialPortProperties "SerialPortProperties"
#define kVSPContollerProperties "VSPController"
#define kVSPDefaultPortCount 4

// Driver instance state resource
struct VSPDriver_IVars {
    IOUserClient*   m_controller;
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
        goto error_exit;
    }

    // Create instance state resource
    ivars = IONewZero(VSPDriver_IVars, 1);
    if (!ivars) {
        VSPLog(LOG_PREFIX, "Unable to allocate driver data.\n");
        result = false;
        goto error_exit;
    }
    
    return true;
    
error_exit:
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
    
    // Create 4 serial port instances with each IOSerialBSDClient sub instance
    if ((ret = CreateSerialPort(provider, kVSPDefaultPortCount)) != kIOReturnSuccess) {
        goto error_exit;
    }

    // Register driver instance to IOReg
    if ((ret = RegisterService()) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "Start: RegisterService failed. code=%d\n", ret);
        goto error_exit;
    }

    VSPLog(LOG_PREFIX, "Start: driver started successfully.\n");
    return kIOReturnSuccess;

error_exit:
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
        port->SetParent(this);

        // save instance for controller
        ivars->m_serialPorts[i] = port;
    }
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// Create the 'VSP Controller' user client instances
//
kern_return_t VSPDriver::CreateUserClient(IOService* provider)
{
    kern_return_t ret;
    IOService* service;

    VSPLog(LOG_PREFIX, "CreateUserClient: create VSP controller from Info.plist.\n");

/*
#if 0
    // ???? requires IUserClient class in Info.plist property
    // ???? "UserClientProperties.IOClass"
    IOUserClient* userClient;
    
    ret = NewUserClient(1, &userClient);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "Start: NewUserClient failed. code=%d\n", ret);

    }
#endif
*/
    
    // Create sub service object from UserClientProperties in Info.plist
    ret= Create(this, kVSPContollerProperties, &service);
    if (ret != kIOReturnSuccess || service == nullptr) {
        VSPLog(LOG_PREFIX, "CreateUserClient: create failed. code=%d\n", ret);
        return ret;
    }
    
    VSPLog(LOG_PREFIX, "CreateUserClient: check IOUserClient type.\n");
    
    // Sane check object type
    ivars->m_controller = OSDynamicCast(IOUserClient, service);
    if (ivars->m_controller == nullptr) {
        VSPLog(LOG_PREFIX, "CreateUserClient: Cast to IOUserClient failed.\n");
        service->release();
        return kIOReturnInvalid;
    }
    
    return kIOReturnSuccess;
}
