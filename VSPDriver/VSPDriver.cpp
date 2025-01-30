//
//  VSPDriver.cpp
//  VSPDriver
//
//  Created by Björn Eschrich on 30.01.25.
//

#include <os/log.h>

#include <DriverKit/IOUserServer.h>
#include <DriverKit/IOLib.h>

#include "VSPDriver.h"

kern_return_t
IMPL(VSPDriver, Start)
{
    kern_return_t ret;
    ret = Start(provider, SUPERDISPATCH);
    os_log(OS_LOG_DEFAULT, "Hello World");
    return ret;
}
