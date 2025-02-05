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

#include <DriverKit/OSDictionary.h>

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
         <key>UserClientProperties</key>
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

// Driver instance state resource
struct VSPDriver_IVars {
    VSPSerialPort* m_serialPort;
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
   
    /* check our private driver instance */
    if (!ivars) {
        VSPLog(LOG_PREFIX, "Start: Private driver instance is NULL\n");
        return kIOReturnInvalid;
    }

    /* call apple style super method */
    ret = Start(provider, SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "Start(super): failed. code=%d\n", ret);
        return ret;
    }

    IOUserClient* userClient;
    
    ret = NewUserClient(1, &userClient);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "Start: NewUserClient failed. code=%d\n", ret);

    }

    if ((ret = LoadSerialPort(provider, 4)) != kIOReturnSuccess) {
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
    
    /* shutdown */
     if ((ret= Stop(provider, SUPERDISPATCH)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "Stop (suprt) failed. code=%d\n", ret);
    } else {
        VSPLog(LOG_PREFIX, "driver successfully removed.\n");
    }
    
    return ret;
}

// --------------------------------------------------------------------
// LoadSerialPort(IOService* provider)
// Load VSPSerialPort instance
kern_return_t VSPDriver::LoadSerialPort(IOService* provider, uint8_t count)
{
    kern_return_t ret;
    IOService* service;

    VSPLog(LOG_PREFIX, "LoadSerialPort: create #%d VSPSerialPort from Info.plist.\n", count);
    
    for (uint8_t i = 0; i < count; i++) {
        // Create sub service object from UserClientProperties in Info.plist
        ret= Create(this, "UserClientProperties", &service);
        if (ret != kIOReturnSuccess || service == nullptr) {
            VSPLog(LOG_PREFIX, "LoadSerialPort: create [%d] failed. code=%d\n", count, ret);
            return ret;
        }
        
        VSPLog(LOG_PREFIX, "LoadSerialPort: check VSPSerialPort type.\n");
        
        // Check object type
        ivars->m_serialPort = OSDynamicCast(VSPSerialPort, service);
        if (ivars->m_serialPort == nullptr) {
            VSPLog(LOG_PREFIX, "LoadSerialPort: Cast to VSPSerialPort failed.\n");
            service->release();
            return kIOReturnInvalid;
        }
    }
    
    return kIOReturnSuccess;
}
