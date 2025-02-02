// ********************************************************************
// VSPDriver - VSPDriver.cpp
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************

// -- OS
#include <os/log.h>

#include <DriverKit/IOLib.h>
#include <DriverKit/IOService.h>
#include <DriverKit/IODataQueueDispatchSource.h>
#include <SerialDriverKit/IOUserSerial.h>

// -- My
#include "VSPLogger.h"
#include "VSPDriver.h"
#include "VSPDriverPrivate.h"

#define LOG_PREFIX "VSPDriver"

// Driver interface state structure
struct VSPDriver_IVars {
    VSPDriverPrivate* p = nullptr;
};

// --------------------------------------------------------------------
//
//
bool VSPDriver::init()
{
    bool result;
    
    VSPLog(LOG_PREFIX, "init called.\n");
    
    if (!(result = super::init())) {
        VSPLog(LOG_PREFIX, "free (super) falsed. result=%d\n", result);
        goto error_exit;
    }

    ivars = IONewZero(VSPDriver_IVars, 1);
    if (!ivars) {
        VSPLog(LOG_PREFIX, "Unable to allocate driver data.\n");
        result = false;
        goto error_exit;
    }

    ivars->p = new VSPDriverPrivate(this);
    if (!ivars->p) {
        VSPLog(LOG_PREFIX, "Unable to allocate private VSP object instance.\n");
        result = false;
        goto error_exit;
    }
    
    return true;
    
error_exit:
    return result;
}

// --------------------------------------------------------------------
//
//
void VSPDriver::free()
{
    VSPLog(LOG_PREFIX, "free called.\n");

    /* release private driver instance */
    if (ivars) {
        if (ivars->p) {
            delete ivars->p;
        }
    }
    
    /* release instance vars */
    IOSafeDeleteNULL(ivars, VSPDriver_IVars, 1);
}

// --------------------------------------------------------------------
// Start_Impl(IOService* provider)
//
kern_return_t IMPL(VSPDriver, Start)
{
    kern_return_t ret;
  
    VSPLog(LOG_PREFIX, "start called.\n");
   
    /* check our private driver instance */
    if (!ivars || !ivars->p) {
        VSPLog(LOG_PREFIX, "Private driver instance is NULL\n");
        return kIOReturnInvalid;
    }

    /* call apple style super method */
    ret = Start(provider, SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "Start (super): failed. code=%d\n", ret);
        return ret;
    }
     
    /* initialize private driver instance */
    if ((ret = ivars->p->Start(provider)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "Start (private): failed. code=%d\n", ret);
        Stop(provider, SUPERDISPATCH);
        return ret;
    }  
    
    if ((ret = RegisterService()) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "RegisterService failed. code=%d\n", ret);
        Stop(provider, SUPERDISPATCH);
        return ret;
    }

    VSPLog(LOG_PREFIX, "driver started successfully.\n");
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// Stop_Impl(IOService* provider)
//
kern_return_t IMPL(VSPDriver, Stop)
{
    kern_return_t ret = kIOReturnIOError;
    
    VSPLog(LOG_PREFIX, "Stop called.\n");

    if (ivars && ivars->p) {
        if ((ret = ivars->p->Stop(provider)) != kIOReturnSuccess) {
            VSPLog(LOG_PREFIX, "Stop (private) failed. code=%d\n", ret);
        }
    }

    ret = Stop(provider, SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "Stop (suprt) failed. code=%d\n", ret);
    } else {
        VSPLog(LOG_PREFIX, "driver successfully removed.\n");
    }
    
    return ret;
}

// --------------------------------------------------------------------
// RxDataAvailable_Impl()
//
void IMPL(VSPDriver, RxDataAvailable)
{
    VSPLog(LOG_PREFIX, "RxDataAvailable called.\n");

    if (ivars && ivars->p) {
        ivars->p->RxDataAvailable();
    }

    RxDataAvailable(SUPERDISPATCH);
}

// --------------------------------------------------------------------
// TxDataAvailable_Impl()
//
void IMPL(VSPDriver, TxDataAvailable)
{
    VSPLog(LOG_PREFIX, "TxDataAvailable called.\n");
    
    if (ivars && ivars->p) {
        if (ivars->p->TxDataAvailable() == kIOReturnSuccess) {
            
        }
    }
}

void IMPL(VSPDriver, TxPacketsAvailable)
{
    VSPLog(LOG_PREFIX, "TxPacketsAvailable called.\n");
    
    if (ivars && ivars->p) {
        if (ivars->p->TxPacketsAvailable(action) == kIOReturnSuccess) {
            TxFreeSpaceAvailable(SUPERDISPATCH);
        }
    }
}

// --------------------------------------------------------------------
// RxFreeSpaceAvailable_Impl()
//
void IMPL(VSPDriver, RxFreeSpaceAvailable)
{
    VSPLog(LOG_PREFIX, "RxFreeSpaceAvailable called.\n");
    if (ivars && ivars->p) {
        ivars->p->RxFreeSpaceAvailable();
    }

    RxFreeSpaceAvailable(SUPERDISPATCH);
}

// --------------------------------------------------------------------
// TxFreeSpaceAvailable_Impl()
//
void IMPL(VSPDriver, TxFreeSpaceAvailable)
{
    VSPLog(LOG_PREFIX, "TxFreeSpaceAvailable called.\n");

    if (ivars && ivars->p) {
        ivars->p->TxFreeSpaceAvailable();
    }

    TxFreeSpaceAvailable(SUPERDISPATCH);
}

// --------------------------------------------------------------------
// SetModemStatus_Impl(bool cts, bool dsr, bool ri, bool dcd)
//
kern_return_t IMPL(VSPDriver, SetModemStatus)
{
    kern_return_t ret = kIOReturnIOError;

    VSPLog(LOG_PREFIX, "SetModemStatus called.\n");

    if (ivars && ivars->p) {
        ret = ivars->p->SetModemStatus(cts, dsr, ri, dcd);
        if (ret != kIOReturnSuccess) {
            VSPLog(LOG_PREFIX, "SetModemStatus (private) failed. code=%d\n", ret);
            return ret;
        }
    }
    
    ret = SetModemStatus(cts, dsr, ri, dcd, SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "SetModemStatus (super) failed. code=%d\n", ret);
        return ret;
    }
    
    return ret;
}

// --------------------------------------------------------------------
// RxError_Impl(bool overrun, bool break, bool framing, bool parity)
//
kern_return_t IMPL(VSPDriver, RxError)
{
    kern_return_t ret = kIOReturnIOError;

    VSPLog(LOG_PREFIX, "RxError called.\n");

    if (ivars && ivars->p) {
        ret = ivars->p->RxError(overrun, gotBreak, framingError, parityError);
        if (ret != kIOReturnSuccess) {
            VSPLog(LOG_PREFIX, "RxError (private) failed. code=%d\n", ret);
            return ret;
        }
    }
    
    ret = RxError(overrun, gotBreak, framingError, parityError, SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "RxError (super) failed. code=%d\n", ret);
        return ret;
    }
    
    return ret;
}

// --------------------------------------------------------------------
// HwActivate_Impl()
//
kern_return_t IMPL(VSPDriver, HwActivate)
{
    kern_return_t ret = kIOReturnIOError;

    VSPLog(LOG_PREFIX, "HwActivate called.\n");

    if (ivars && ivars->p) {
        ret = ivars->p->HwActivate();
        if (ret != kIOReturnSuccess) {
            VSPLog(LOG_PREFIX, "HwActivate (private) failed. code=%d\n", ret);
            return ret;
        }
    }
    
    ret = HwActivate(SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "HwActivate (super) failed. code=%d\n", ret);
        return ret;
    }
    
    return ret;
}

// --------------------------------------------------------------------
// HwActivate_Impl()
//
kern_return_t IMPL(VSPDriver, HwDeactivate)
{
    kern_return_t ret = kIOReturnIOError;

    VSPLog(LOG_PREFIX, "HwDeactivate called.\n");

    if (ivars && ivars->p) {
        ret = ivars->p->HwDeactivate();
        if (ret != kIOReturnSuccess) {
            VSPLog(LOG_PREFIX, "HwDeactivate (private) failed. code=%d\n", ret);
            return ret;
        }
    }
    
    ret = HwDeactivate(SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "HwDeactivate (super) failed. code=%d\n", ret);
        return ret;
    }
    
    return ret;
}

// --------------------------------------------------------------------
// HwResetFIFO_Impl()
//
kern_return_t IMPL(VSPDriver, HwResetFIFO)
{
    kern_return_t ret = kIOReturnIOError;

    VSPLog(LOG_PREFIX, "HwResetFIFO called.\n");

    if (ivars && ivars->p) {
        ret = ivars->p->HwResetFIFO(tx, rx);
        if (ret != kIOReturnSuccess) {
            VSPLog(LOG_PREFIX, "HwResetFIFO (private) failed. code=%d\n", ret);
            return ret;
        }
    }
    
    return ret;
}

// --------------------------------------------------------------------
// HwSendBreak_Impl()
//
kern_return_t IMPL(VSPDriver, HwSendBreak)
{
    kern_return_t ret = kIOReturnIOError;

    VSPLog(LOG_PREFIX, "HwSendBreak called.\n");

    if (ivars && ivars->p) {
        ret = ivars->p->HwSendBreak(sendBreak);
        if (ret != kIOReturnSuccess) {
            VSPLog(LOG_PREFIX, "HwSendBreak (private) failed. code=%d\n", ret);
            return ret;
        }
    }

    return ret;
}

// --------------------------------------------------------------------
// HwGetModemStatus_Impl()
//
kern_return_t IMPL(VSPDriver, HwGetModemStatus)
{
    kern_return_t ret = kIOReturnIOError;

    VSPLog(LOG_PREFIX, "HwGetModemStatus called.\n");

    if (ivars && ivars->p) {
        ret = ivars->p->HwGetModemStatus(cts, dsr, ri, dcd);
        if (ret != kIOReturnSuccess) {
            VSPLog(LOG_PREFIX, "HwGetModemStatus (private) failed. code=%d\n", ret);
            return ret;
        }
    }
    
    return ret;
}

// --------------------------------------------------------------------
// HwProgramUART_Impl()
//
kern_return_t IMPL(VSPDriver, HwProgramUART)
{
    kern_return_t ret = kIOReturnIOError;

    VSPLog(LOG_PREFIX, "HwProgramUART called.\n");

    if (ivars && ivars->p) {
        ret = ivars->p->HwProgramUART(baudRate, nDataBits, nHalfStopBits, parity);
        if (ret != kIOReturnSuccess) {
            VSPLog(LOG_PREFIX, "HwProgramUART (private) failed. code=%d\n", ret);
            return ret;
        }
    }
    
    return ret;
}

// --------------------------------------------------------------------
// HwProgramBaudRate_Impl()
//
kern_return_t IMPL(VSPDriver, HwProgramBaudRate)
{
    kern_return_t ret = kIOReturnIOError;

    VSPLog(LOG_PREFIX, "HwProgramBaudRate called.\n");

    if (ivars && ivars->p) {
        ret = ivars->p->HwProgramBaudRate(baudRate);
        if (ret != kIOReturnSuccess) {
            VSPLog(LOG_PREFIX, "HwProgramBaudRate (private) failed. code=%d\n", ret);
            return ret;
        }
    }
    
    return ret;
}

// --------------------------------------------------------------------
// HwProgramMCR_Impl()
//
kern_return_t IMPL(VSPDriver, HwProgramMCR)
{
    kern_return_t ret = kIOReturnIOError;

    VSPLog(LOG_PREFIX, "HwProgramMCR called.\n");

    if (ivars && ivars->p) {
        ret = ivars->p->HwProgramMCR(dtr, rts);
        if (ret != kIOReturnSuccess) {
            VSPLog(LOG_PREFIX, "HwProgramMCR (private) failed. code=%d\n", ret);
            return ret;
        }
    }
    
    return ret;
}

// --------------------------------------------------------------------
// HwProgramLatencyTimer_Impl()
//
kern_return_t IMPL(VSPDriver, HwProgramLatencyTimer)
{
    kern_return_t ret = kIOReturnIOError;

    VSPLog(LOG_PREFIX, "HwProgramLatencyTimer called.\n");

    if (ivars && ivars->p) {
        ret = ivars->p->HwProgramLatencyTimer(latency);
        if (ret != kIOReturnSuccess) {
            VSPLog(LOG_PREFIX, "HwProgramLatencyTimer (private) failed. code=%d\n", ret);
            return ret;
        }
    }
    
    return ret;
}

// --------------------------------------------------------------------
// HwProgramLatencyTimer_Impl()
//
kern_return_t IMPL(VSPDriver, HwProgramFlowControl)
{
    kern_return_t ret = kIOReturnIOError;

    VSPLog(LOG_PREFIX, "HwProgramFlowControl called.\n");

    if (ivars && ivars->p) {
        ret = ivars->p->HwProgramFlowControl(arg, xon, xoff);
        if (ret != kIOReturnSuccess) {
            VSPLog(LOG_PREFIX, "HwProgramFlowControl (private) failed. code=%d\n", ret);
            return ret;
        }
    }

    return ret;
}
