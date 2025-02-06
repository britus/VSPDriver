// ********************************************************************
// VSPSerialPort - Serial port implementation
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************

// -- OS
#include <stdio.h>
#include <os/log.h>

#include <DriverKit/OSDictionary.h>
#include <DriverKit/OSData.h>
#include <DriverKit/OSString.h>
#include <DriverKit/OSArray.h>
#include <DriverKit/OSSet.h>
#include <DriverKit/OSOrderedSet.h>
#include <DriverKit/IOLib.h>
#include <DriverKit/IOService.h>
#include <DriverKit/IOTypes.h>
#include <DriverKit/IOMemoryMap.h>
#include <DriverKit/IOMemoryDescriptor.h>
#include <DriverKit/IOBufferMemoryDescriptor.h>
#include <DriverKit/IODispatchQueue.h>
#include <DriverKit/IODispatchSource.h>

#include <DriverKit/IODataQueueDispatchSource.h>
#include <DriverKit/IOInterruptDispatchSource.h>

// -- SerialDriverKit
#include <SerialDriverKit/SerialDriverKit.h>
#include <SerialDriverKit/SerialPortInterface.h>
#include <SerialDriverKit/IOUserSerial.h>

// -- My
#include "VSPLogger.h"
#include "VSPSerialPort.h"
#include "VSPDriver.h"

#define LOG_PREFIX "VSPSerialPort"

#define TTY_BASE_NAME "vsp"

#ifndef IOLockFreeNULL
#define IOLockFreeNULL(l) { if (NULL != (l)) { IOLockFree(l); (l) = NULL; } }
#endif

typedef struct {
    bool cts;
    bool dsr;
    bool ri;
    bool dcd;
} THwSerialStatus;

typedef struct{
    uint32_t arg;
    uint8_t xon;
    uint8_t xoff;
} THwFlowControl;

typedef struct {
    bool dtr;
    bool rts;
} THwMCR;

typedef struct {
    uint32_t baudRate;
    uint8_t nDataBits;
    uint8_t nHalfStopBits;
    uint8_t parity;
} TUartParameters;

typedef struct {
    bool overrun;
    bool gotBreak;
    bool framingError;
    bool parityError;
} TErrorState;

// Driver instance state resource
struct VSPSerialPort_IVars {
    IOService* m_provider = nullptr;
    VSPDriver* m_parent = nullptr;

    IOLock* m_lock = nullptr;                       // for resource locking

    /* OS provided memory descriptors by overridden
     * method ConnectQueues(...) */
    IOBufferMemoryDescriptor *m_ifmd = nullptr;     // Interrupt related
    IOBufferMemoryDescriptor *m_txqmd = nullptr;    // OS -> HW Transmit
    IOBufferMemoryDescriptor *m_rxqmd = nullptr;    // HW -> OS Receive
    
    /* Response buffer created by TxAvailable() */
    OSData* m_rxData = nullptr;
    
    // Serial interface
    TErrorState m_errorState = {};
    TUartParameters m_uartParams = {};
    THwSerialStatus m_hwStatus = {};
    THwFlowControl m_hwFlowControl = {};
    THwMCR m_hwMCR = {};
    uint32_t m_hwLatency = 0;
    
    bool m_txIsComplete = false;
};

// --------------------------------------------------------------------
// Allocate internal resources
//
bool VSPSerialPort::init(void)
{
    bool result;
    
    VSPLog(LOG_PREFIX, "init called.\n");
    
    if (!(result = super::init())) {
        VSPLog(LOG_PREFIX, "free (super) falsed. result=%d\n", result);
        goto error_exit;
    }
    
    // Create instance state resource
    ivars = IONewZero(VSPSerialPort_IVars, 1);
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
void VSPSerialPort::free(void)
{
    VSPLog(LOG_PREFIX, "free called.\n");
    
    // Release instance state resource
    IOSafeDeleteNULL(ivars, VSPSerialPort_IVars, 1);
    super::free();
}

// --------------------------------------------------------------------
// Start_Impl(IOService* provider)
//
kern_return_t IMPL(VSPSerialPort, Start)
{
    kern_return_t ret;
    
    VSPLog(LOG_PREFIX, "Start: called.\n");
    
    // sane check our driver instance vars
    if (!ivars) {
        VSPLog(LOG_PREFIX, "Start: Private driver instance is NULL\n");
        return kIOReturnInvalid;
    }
    
    // call super method (apple style)
    ret = Start(provider, SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "Start(super): failed. code=%d\n", ret);
        return ret;
    }
    
    // remember OS provider
    ivars->m_provider = provider;

    // the resource locker
    ivars->m_lock = IOLockAlloc();
    if (ivars->m_lock == nullptr) {
        VSPLog(LOG_PREFIX, "Start: Unable to allocate lock object.\n");
        goto error_exit;
    }
    
    // create our own TTY name and index
    if ((ret = SetupTTYBaseName()) != kIOReturnSuccess) {
        goto error_exit;
    }

    // default UART parameters
    ivars->m_uartParams.baudRate = 112500;
    ivars->m_uartParams.nHalfStopBits = 2;
    ivars->m_uartParams.nDataBits = 8;
    ivars->m_uartParams.parity = 0;

    VSPLog(LOG_PREFIX, "Start: register service.\n");

    // Register driver instance to IOReg
    if ((ret = RegisterService()) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "Start: RegisterService failed. code=%d\n", ret);
        goto error_exit;
    }
    
    VSPLog(LOG_PREFIX, "Start: driver started successfully.\n");
    return kIOReturnSuccess;
    
error_exit:
    CleanupResources();
    Stop(provider, SUPERDISPATCH);
    return ret;
}

// --------------------------------------------------------------------
// Stop_Impl(IOService* provider)
//
kern_return_t IMPL(VSPSerialPort, Stop)
{
    kern_return_t ret;

    VSPLog(LOG_PREFIX, "Stop called.\n");
    
    // unlink VSP parent
    ivars->m_parent = nullptr;
    
    // remove all IVars resources
    CleanupResources();
    
    /* shutdown */
    if ((ret= Stop(provider, SUPERDISPATCH)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "Stop (super) failed. code=%d\n", ret);
    } else {
        VSPLog(LOG_PREFIX, "driver successfully removed.\n");
    }
    
    return ret;
}

// --------------------------------------------------------------------
// ConnectQueues_Impl( ... )
//
kern_return_t IMPL(VSPSerialPort, ConnectQueues)
{
    VSPLog(LOG_PREFIX, "ConnectQueues called\n");

    IOReturn ret = ConnectQueues(ifmd, rxqmd, txqmd,
                                 in_rxqmd,
                                 in_txqmd,
                                 in_rxqoffset,
                                 in_txqoffset,
                                 in_rxqlogsz,
                                 in_txqlogsz, SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "ConnectQueues (super): failed. code=%d\n", ret);
        return ret;
    }
    
    //-- sane check --//
    
    ivars->m_ifmd = OSDynamicCast(IOBufferMemoryDescriptor, (*ifmd));
    if (ivars->m_ifmd == nullptr) {
        VSPLog(LOG_PREFIX, "ConnectQueues: Invalid interrupt memory descriptor detected.\n");
        return kIOReturnInvalid;
    }
    
    ivars->m_txqmd = OSDynamicCast(IOBufferMemoryDescriptor, (*txqmd));
    if (ivars->m_txqmd == nullptr) {
        VSPLog(LOG_PREFIX, "ConnectQueues: Invalid TX memory descriptor detected.\n");
        return kIOReturnInvalid;
    }

    ivars->m_rxqmd = OSDynamicCast(IOBufferMemoryDescriptor, (*rxqmd));
    if (ivars->m_rxqmd == nullptr) {
        VSPLog(LOG_PREFIX, "ConnectQueues: Invalid RX memory descriptor detected.\n");
        return kIOReturnInvalid;
    }

    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// RxDataAvailable_Impl()
//
void IMPL(VSPSerialPort, RxDataAvailable)
{
    IOAddressSegment seg;
    const void* rxbuff;
    char* buffer;
    uint64_t size;
    IOReturn ret;

    VSPLog(LOG_PREFIX, "RxDataAvailable called.\n");
    
    if (!ivars->m_rxData)
        return;
    // Lock to ensure thread safety
    IOLockLock(ivars->m_lock);
    
    // get the address of the TX ring buffer
    if ((ret = ivars->m_rxqmd->GetAddressRange(&seg)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "RxDataAvailable: RX GetAddressRange failed. code=%d\n", ret);
        goto finished;
    }
    
    buffer = (char*) seg.address;
    size   = seg.length;
    
    VSPLog(LOG_PREFIX, "RxDataAvailable: get m_rxData buffer\n");
    
    /* get rx buffer pointer */
    rxbuff = ivars->m_rxData->getBytesNoCopy(0, size);

    VSPLog(LOG_PREFIX, "RxDataAvailable: copy m_rxData to rxqmd buffer\n");

    /* copy data to rxqmd memory descriptor */
    memcpy(buffer, rxbuff, size);
    
    VSPLog(LOG_PREFIX, "RxDataAvailable: Dump m_rxqmd -------------\n");
    VSPLog(LOG_PREFIX, "RxDataAvailable> buffer=0x%llx size=%llu\n",
           (uint64_t) buffer, size);
    
    // !! Debug ....
    for (uint64_t i = 0; i < size && i < 16; i++) {
        VSPLog(LOG_PREFIX, "TxDataAvailable> buffer[%02lld]=0x%02x %c\n", i, buffer[i], buffer[i]);
    }

    RxDataAvailable(SUPERDISPATCH);

finished:
    OSSafeReleaseNULL(ivars->m_rxData);
    IOLockUnlock(ivars->m_lock);
}

// --------------------------------------------------------------------
// TxDataAvailable_Impl()
//
void IMPL(VSPSerialPort, TxDataAvailable)
{
    IOAddressSegment seg;
    char* buffer;
    uint64_t size;
    IOReturn ret;
    
    VSPLog(LOG_PREFIX, "TxDataAvailable called.\n");
    
    // Lock to ensure thread safety
    IOLockLock(ivars->m_lock);

    // reset
    ivars->m_txIsComplete = false;

    // get the address of the TX ring buffer
    if ((ret = ivars->m_txqmd->GetAddressRange(&seg)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "TxDataAvailable: TX GetAddressRange failed. code=%d\n", ret);
        goto finished;
    }
    
    buffer = (char*) seg.address;
    size   = seg.length;
    
    VSPLog(LOG_PREFIX, "TxDataAvailable: Dump m_txqmd -------------\n");
    VSPLog(LOG_PREFIX, "TxDataAvailable> buffer=0x%llx size=%llu\n",
           (uint64_t) buffer, size);
    
    // !! Debug ....
    for (uint64_t i = 0; i < size && i < 16; i++) {
        VSPLog(LOG_PREFIX, "TxDataAvailable> buffer[%02lld]=0x%02x %c\n", i, buffer[i], buffer[i]);
    }

    if (!ivars->m_rxData) {
        ivars->m_rxData = OSData::withBytes(buffer, size);
    }
    
    // response incomming to RX
    RxDataAvailable();
    
    // notify completion
    TxFreeSpaceAvailable();

finished:
    IOLockUnlock(ivars->m_lock);
}

// --------------------------------------------------------------------
// RxFreeSpaceAvailable_Impl()
//
void IMPL(VSPSerialPort, RxFreeSpaceAvailable)
{
    VSPLog(LOG_PREFIX, "RxFreeSpaceAvailable called.\n");
    
    RxFreeSpaceAvailable(SUPERDISPATCH);
}

// --------------------------------------------------------------------
// TxFreeSpaceAvailable_Impl()
//
void IMPL(VSPSerialPort, TxFreeSpaceAvailable)
{
    VSPLog(LOG_PREFIX, "TxFreeSpaceAvailable called.\n");
    
    TxFreeSpaceAvailable(SUPERDISPATCH);
}

// --------------------------------------------------------------------
// SetModemStatus_Impl(bool cts, bool dsr, bool ri, bool dcd)
//
kern_return_t IMPL(VSPSerialPort, SetModemStatus)
{
    IOReturn ret;
    
    VSPLog(LOG_PREFIX, "SetModemStatus called.\n");
    VSPLog(LOG_PREFIX, "CTS=%d DSR=%d RI=%d DCD=%d\n", cts, dsr, ri, dcd);
    
    ret = SetModemStatus(cts, dsr, ri, dcd, SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "SetModemStatus (super) failed. code=%d\n", ret);
        return ret;
    }
    
    ivars->m_hwStatus.cts = cts;
    ivars->m_hwStatus.dsr = dsr;
    ivars->m_hwStatus.ri  = ri;
    ivars->m_hwStatus.dcd = dcd;
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// RxError_Impl(bool overrun, bool break, bool framing, bool parity)
//
kern_return_t IMPL(VSPSerialPort, RxError)
{
    kern_return_t ret;
    
    VSPLog(LOG_PREFIX, "RxError called.\n");
    
    if (overrun) {
        VSPLog(LOG_PREFIX, "RX overrun.\n");
    }
    
    if (gotBreak) {
        VSPLog(LOG_PREFIX, "RX got break.\n");
    }
    
    if (framingError) {
        VSPLog(LOG_PREFIX, "RX framing error.\n");
    }
    
    if (parityError) {
        VSPLog(LOG_PREFIX, "RX parity error.\n");
    }
    
    ivars->m_errorState.overrun = overrun;
    ivars->m_errorState.framingError = framingError;
    ivars->m_errorState.gotBreak = gotBreak;
    ivars->m_errorState.parityError = parityError;
    
    ret = RxError(overrun, gotBreak, framingError, parityError, SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "RxError (super) failed. code=%d\n", ret);
        return ret;
    }

    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// HwActivate_Impl()
//
kern_return_t IMPL(VSPSerialPort, HwActivate)
{
    kern_return_t ret = kIOReturnIOError;
    
    VSPLog(LOG_PREFIX, "HwActivate called.\n");
    
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
kern_return_t IMPL(VSPSerialPort, HwDeactivate)
{
    kern_return_t ret = kIOReturnIOError;
    
    VSPLog(LOG_PREFIX, "HwDeactivate called.\n");
    
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
kern_return_t IMPL(VSPSerialPort, HwResetFIFO)
{
    VSPLog(LOG_PREFIX, "HwResetFIFO called.\n");
    VSPLog(LOG_PREFIX, "HwResetFIFO: tx=%d rx=%d\n", tx, rx);
    
    if (rx && ivars->m_rxData) {
        OSSafeReleaseNULL(ivars->m_rxData);
    }
    
    // TODO: FIFO implement reset
    /*if (rx) fifoResetRx(); */
    /*if (tx) fifoResetTx(); */
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// HwSendBreak_Impl()
//
kern_return_t IMPL(VSPSerialPort, HwSendBreak)
{
    VSPLog(LOG_PREFIX, "HwSendBreak called.\n");
    VSPLog(LOG_PREFIX, "HwSendBreak: sendBreak=%d\n", sendBreak);
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// HwGetModemStatus_Impl()
//
kern_return_t IMPL(VSPSerialPort, HwGetModemStatus)
{
    VSPLog(LOG_PREFIX, "HwGetModemStatus called.\n");
    VSPLog(LOG_PREFIX, "HwGetModemStatus: cts=%d dsr=%d ri=%d dcd=%d\n", //
           ivars->m_hwStatus.cts, ivars->m_hwStatus.dsr, //
           ivars->m_hwStatus.ri, ivars->m_hwStatus.dcd);
    
    if (cts != nullptr) {
        (*cts) = ivars->m_hwStatus.cts;
    }
    
    if (dsr != nullptr) {
        (*dsr) = ivars->m_hwStatus.dsr;
    }
    
    if (ri != nullptr) {
        (*ri) = ivars->m_hwStatus.ri;
    }
    
    if (dcd != nullptr) {
        (*dcd) = ivars->m_hwStatus.dcd;
    }
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// HwProgramUART_Impl()
//
kern_return_t IMPL(VSPSerialPort, HwProgramUART)
{
    VSPLog(LOG_PREFIX, "HwProgramUART called.\n");
    VSPLog(LOG_PREFIX, "HwProgramUART: baudRate=%d nDataBits=%d nHalfStopBits=%d parity=%d\n",
           baudRate, nDataBits, nHalfStopBits, parity);
    
    ivars->m_uartParams.baudRate = baudRate;
    ivars->m_uartParams.nDataBits = nDataBits;
    ivars->m_uartParams.nHalfStopBits = nHalfStopBits;
    ivars->m_uartParams.parity = parity;
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// HwProgramBaudRate_Impl()
//
kern_return_t IMPL(VSPSerialPort, HwProgramBaudRate)
{
    VSPLog(LOG_PREFIX, "HwProgramBaudRate called.\n");
    VSPLog(LOG_PREFIX, "HwProgramBaudRate: baudRate=%d\n", baudRate);
    
    ivars->m_uartParams.baudRate = baudRate;
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// HwProgramMCR_Impl()
//
kern_return_t IMPL(VSPSerialPort, HwProgramMCR)
{
    VSPLog(LOG_PREFIX, "HwProgramMCR called.\n");
    VSPLog(LOG_PREFIX, "HwProgramMCR: dtr=%d rts=%d\n", dtr, rts);
    
    ivars->m_hwMCR.dtr = dtr;
    ivars->m_hwMCR.rts = rts;
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// HwProgramLatencyTimer_Impl()
//
kern_return_t IMPL(VSPSerialPort, HwProgramLatencyTimer)
{
    VSPLog(LOG_PREFIX, "HwProgramLatencyTimer called.\n");
    VSPLog(LOG_PREFIX, "HwProgramLatencyTimer: latency=%d\n", latency);
    
    ivars->m_hwLatency = latency;
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// HwProgramLatencyTimer_Impl()
//
kern_return_t IMPL(VSPSerialPort, HwProgramFlowControl)
{
    VSPLog(LOG_PREFIX, "HwProgramFlowControl called.\n");
    VSPLog(LOG_PREFIX, "HwProgramFlowControl: arg=%02x xon=%02x xoff=%02x\n", arg, xon, xoff);
    
    ivars->m_hwFlowControl.arg = arg;
    ivars->m_hwFlowControl.xon = xon;
    ivars->m_hwFlowControl.xoff = xoff;
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// Remove all resources in IVars
//
void VSPSerialPort::CleanupResources()
{
    VSPLog(LOG_PREFIX, "CleanupResources called.\n");
    
    // Disconnect all queues. This deallocates
    // the INT/RX/TX MDs resources too
    this->DisconnectQueues();
    
    IOLockFreeNULL(ivars->m_lock);
}

// --------------------------------------------------------------------
//
//
void VSPSerialPort::SetParent(VSPDriver* parent)
{
    if (ivars != nullptr) {
        ivars->m_parent = parent;
    }
}

// --------------------------------------------------------------------
//
//
IOReturn VSPSerialPort::SetupTTYBaseName()
{
    IOReturn ret;
    OSDictionary* properties = nullptr;
    OSString* baseName = nullptr;
    
    // setup custom TTY name
    if ((ret = CopyProperties(&properties)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "Start: Unable to get driver properties. code=%d\n", ret);
        return ret;
    }
    
    baseName = OSString::withCString(TTY_BASE_NAME);
    properties->setObject(kIOTTYBaseNameKey, baseName);
    
    // write back to driver instance
    if ((ret = SetProperties(properties)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "Start: Unable to set TTY base name. code=%d\n", ret);
        //return ret; // ??? an error - why???
    }
    
    OSSafeReleaseNULL(baseName);
    OSSafeReleaseNULL(properties);
    return kIOReturnSuccess;
}
