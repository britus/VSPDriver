// ********************************************************************
// VSPDriver - VSPDriver.cpp
//
// Copyright © 2025 by EoF Software Labs
// SPDX-License-Identifier: MIT
// ********************************************************************

// -- OS
#include <os/log.h>
#include <DriverKit/IOLib.h>

#include "VSPDriver.h"
#include "VSPDriverPrivate.h"

#define DRIVER_NAME "VSP"

/** --------------------------------------------------------
 *
 */
bool VSPDriver::init()
{
    bool result = false;
    
    os_log(OS_LOG_DEFAULT, "[%s] init called.\n", DRIVER_NAME);
    
    if (!(result = super::init())) {
        os_log(OS_LOG_DEFAULT, "[%s] free (super) falsed. result=%d\n", DRIVER_NAME, result);
        goto exit_fail;
    }

    ivars = (VSPDriver_IVars*) IOMallocZero(sizeof(VSPDriver_IVars));
    if (!ivars) {
        os_log(OS_LOG_DEFAULT, "[%s] Unable to instance data.\n", DRIVER_NAME);
        result = false;
        goto exit_fail;
    }

    ivars->c = 0;
    ivars->p = new VSPDriverPrivate(this);
    if (!ivars->p) {
        os_log(OS_LOG_DEFAULT, "[%s] Unable to allocate private VSP instance.\n", DRIVER_NAME);
        result = false;
        goto exit_fail;
    }

exit_fail:
    return result;
}

/** --------------------------------------------------------
 *
 */
void VSPDriver::free()
{
    os_log(OS_LOG_DEFAULT, "[%s] free called.\n", DRIVER_NAME);

    if (ivars) {
        /* release private driver instance */
        if (ivars && ivars->p) {
            delete ivars->p;
            ivars->p = 0L;
        }
        /* release instance vars */
        IOFree(ivars, sizeof(VSPDriver_IVars));
    }
}

/** --------------------------------------------------------
 *
 */
kern_return_t IMPL(VSPDriver, Start)
{
    kern_return_t ret = kIOReturnSuccess;
  
    os_log(OS_LOG_DEFAULT, "[%s] start called.\n", DRIVER_NAME);
   
    /* check our private driver instance */
    if (!ivars || !ivars->p) {
        os_log(OS_LOG_DEFAULT, "[%s] Private driver instance is NULL\n", DRIVER_NAME);
        return kIOReturnInvalid;
    }

    /* call apple style super method */
    ret = Start(provider, SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        os_log(OS_LOG_DEFAULT, "[%s] Start (super) failed. code=%d\n", DRIVER_NAME, ret);
        return ret;
    }

    if ((ret = ivars->p->Start(provider)) != kIOReturnSuccess) {
        os_log(OS_LOG_DEFAULT, "[%s] Start (private) failed. code=%d\n", DRIVER_NAME, ret);
        return ret;
    }
    
    SetName("vsp0");
    
    os_log(OS_LOG_DEFAULT, "[%s] driver started successfully.\n", DRIVER_NAME);
    return kIOReturnSuccess;
}

/** --------------------------------------------------------
 *
 */
kern_return_t IMPL(VSPDriver, Stop) {
    kern_return_t ret;
    
    os_log(OS_LOG_DEFAULT, "[%s] Stop called.\n", DRIVER_NAME);
    
    if (ivars && ivars->p) {
        if ((ret = ivars->p->Stop(provider)) != kIOReturnSuccess) {
            os_log(OS_LOG_DEFAULT, "[%s] Stop (private) failed. code=%d\n", DRIVER_NAME, ret);
        }
    }
    
    ret = Stop(provider, SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        os_log(OS_LOG_DEFAULT, "[%s] Stop (suprt) failed. code=%d\n", DRIVER_NAME, ret);
    }
    else {
        os_log(OS_LOG_DEFAULT, "[%s] driver removed.\n", DRIVER_NAME);
    }
    
    return ret;
}

/** --------------------------------------------------------
 *
 */
void IMPL(VSPDriver, RxDataAvailable)
{
    os_log(OS_LOG_DEFAULT, "[%s] RxDataAvailable called.\n", DRIVER_NAME);

    if (ivars && ivars->p) {
        ivars->p->RxDataAvailable();
    }

    RxDataAvailable(SUPERDISPATCH);
}

/** --------------------------------------------------------
 *
 */
void IMPL(VSPDriver, TxFreeSpaceAvailable)
{
    os_log(OS_LOG_DEFAULT, "[%s] TxFreeSpaceAvailable called.\n", DRIVER_NAME);

    if (ivars && ivars->p) {
        ivars->p->TxFreeSpaceAvailable();
    }

    TxFreeSpaceAvailable(SUPERDISPATCH);
}

/** --------------------------------------------------------
 *
 */
kern_return_t IMPL(VSPDriver, SetModemStatus)
{
    kern_return_t ret;

    os_log(OS_LOG_DEFAULT, "[%s] SetModemStatus called.\n", DRIVER_NAME);

    if (ivars && ivars->p) {
        ret = ivars->p->SetModemStatus(cts, dsr, ri, dcd);
        if (ret != kIOReturnSuccess) {
            os_log(OS_LOG_DEFAULT, "[%s] SetModemStatus (private) failed. code=%d\n", DRIVER_NAME, ret);
            return ret;
        }
    }
    
    ret = SetModemStatus(cts, dsr, ri, dcd, SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        os_log(OS_LOG_DEFAULT, "[%s] SetModemStatus (super) failed. code=%d\n", DRIVER_NAME, ret);
        return ret;
    }
    
    return ret;
}

/** --------------------------------------------------------
 *
 */
kern_return_t IMPL(VSPDriver, RxError)
{
    kern_return_t ret;

    os_log(OS_LOG_DEFAULT, "[%s] RxError called.\n", DRIVER_NAME);

    if (ivars && ivars->p) {
        ret = ivars->p->RxError(overrun, gotBreak, framingError, parityError);
        if (ret != kIOReturnSuccess) {
            os_log(OS_LOG_DEFAULT, "[%s] RxError (private) failed. code=%d\n", DRIVER_NAME, ret);
            return ret;
        }
    }
    
    ret = RxError(overrun, gotBreak, framingError, parityError, SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        os_log(OS_LOG_DEFAULT, "[%s] RxError (super) failed. code=%d\n", DRIVER_NAME, ret);
        return ret;
    }
    
    return ret;
}
