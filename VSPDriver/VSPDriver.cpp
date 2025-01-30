//
//  VSPDriver.cpp
//  VSPDriver
//
//  Created by Björn Eschrich on 30.01.25.
//

// -- OS
#include <os/log.h>
#include "VSPDriver.h"
#include "VSPDriverPrivate.h"

#define DEVICE_NAME "VSP"

/* private singelton driver instance */
static VSPDriverPrivate* g_driver = 0;

kern_return_t IMPL(VSPDriver, Start)
{
    kern_return_t ret = kIOReturnSuccess;
  
    os_log(OS_LOG_DEFAULT, "[%s] start called.\n", DEVICE_NAME);
    
    /* call apple style super method */
    ret = Start(provider, SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        return ret;
    }

    /* create our private driver instance */
    g_driver = new VSPDriverPrivate(this);
    if (!g_driver) {
        return kIOReturnNoSpace;
    }

    if ((ret = g_driver->Start(provider)) != kIOReturnSuccess) {
        return ret;
    }
    
    os_log(OS_LOG_DEFAULT, "[%s] driver started successfully.\n", DEVICE_NAME);
    return kIOReturnSuccess;
}

kern_return_t IMPL(VSPDriver, Stop) {
    os_log(OS_LOG_DEFAULT, "[%s] Stop called.\n", DEVICE_NAME);
    
    if (g_driver) {
        g_driver->Stop(provider);
        delete g_driver;
    }
    
    os_log(OS_LOG_DEFAULT, "[%s] driver removed.\n", DEVICE_NAME);
    return Stop(provider, SUPERDISPATCH);
}
