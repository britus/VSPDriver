// ********************************************************************
// VSPSerialPort - Serial port implementation
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************

// -- OS
#include <os/log.h>
#include <stdio.h>
#include <time.h>

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
#include <DriverKit/IOTimerDispatchSource.h>

// -- SerialDriverKit
#include <SerialDriverKit/SerialDriverKit.h>
#include <SerialDriverKit/SerialPortInterface.h>
#include <SerialDriverKit/IOUserSerial.h>

// -- My
#include "VSPLogger.h"
#include "VSPSerialPort.h"
#include "VSPDriver.h"

#define LOG_PREFIX "VSPSerialPort"

#define kVSPTTYBaseName "vsp"
#define kRxDataQueueName "rxDataQueue"

#ifndef IOLockFreeNULL
#define IOLockFreeNULL(l) { if (NULL != (l)) { IOLockFree(l); (l) = NULL; } }
#endif

// Updated by SetModemStatus read by HwGetModemStatus
typedef struct {
    bool cts;
    bool dsr;
    bool ri;
    bool dcd;
} THwSerialStatus;

// Updated by HwProgramFlowControl
typedef struct{
    uint32_t arg;
    uint8_t xon;
    uint8_t xoff;
} THwFlowControl;

// Updated by HwProgramMCR
typedef struct {
    bool dtr;
    bool rts;
} THwMCR;

// Updated by HwProgramUART and HwProgramBaudRate
typedef struct {
    uint32_t baudRate;
    uint8_t nDataBits;
    uint8_t nHalfStopBits;
    uint8_t parity;
} TUartParameters;

// Updated by RxError and HwSendBreak
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
    
    // Timed events
    IODispatchQueue* m_tiQueue = nullptr;
    IOTimerDispatchSource* m_tiSource = nullptr;
    OSAction* m_tiAction = nullptr;
    OSData* m_tiData = nullptr;

    // Response buffer created by TxAvailable()
    IODispatchQueue* m_rxQueue = nullptr;
    IODataQueueDispatchSource* m_rxSource = nullptr;
    OSAction* m_rxAction = nullptr;

    // Serial interface
    TErrorState m_errorState = {};
    TUartParameters m_uartParams = {};
    THwSerialStatus m_hwStatus = {};
    THwFlowControl m_hwFlowControl = {};
    THwMCR m_hwMCR = {};
    uint32_t m_hwLatency = 5;
    
    bool m_txIsComplete = false;
    bool m_hwActivated = false;
};

using namespace driverkit;
using namespace serial;

// --------------------------------------------------------------------
// Allocate internal resources
//
bool VSPSerialPort::init(void)
{
    bool result;
    
    VSPLog(LOG_PREFIX, "init called.\n");
    
    if (!(result = super::init())) {
        VSPLog(LOG_PREFIX, "super::init falsed. result=%d\n", result);
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
    if ((ret = setupTTYBaseName()) != kIOReturnSuccess) {
        goto error_exit;
    }

    // default UART parameters
    ivars->m_uartParams.baudRate = 112500;
    ivars->m_uartParams.nHalfStopBits = 2;
    ivars->m_uartParams.nDataBits = 8;
    ivars->m_uartParams.parity = 0;

    VSPLog(LOG_PREFIX, "Start: Allocate timer queue and resources.\n");

    ret = IODispatchQueue::Create("kVSPTimerQueue", 0, 0, &ivars->m_tiQueue);
    if (ret != kIOReturnSuccess || !ivars->m_tiQueue) {
        VSPLog(LOG_PREFIX, "Start: Unable to create timer queue. code=%d\n", ret);
        goto error_exit;
    }

    ret = IOTimerDispatchSource::Create(ivars->m_tiQueue, &ivars->m_tiSource);
    if (ret != kIOReturnSuccess || !ivars->m_tiSource) {
        VSPLog(LOG_PREFIX, "Start: Unable to create timer queue. code=%d\n", ret);
        goto error_exit;
    }

    ret = CreateActionNotifyRXReady(sizeof(ivars->m_tiData), &ivars->m_tiAction);
    if (ret != kIOReturnSuccess || !ivars->m_tiAction) {
        VSPLog(LOG_PREFIX, "Start: Unable to timer callback action. code=%d\n", ret);
        goto error_exit;
    }

    ret = ivars->m_tiSource->SetHandler(ivars->m_tiAction);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "Start: Unable to assign timer action. code=%d\n", ret);
        goto error_exit;
    }

    VSPLog(LOG_PREFIX, "Start: register service.\n");

    // Register driver instance to IOReg
    if ((ret = RegisterService()) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "Start: RegisterService failed. code=%d\n", ret);
        goto error_exit;
    }
    
    VSPLog(LOG_PREFIX, "Start: driver started successfully.\n");
    return kIOReturnSuccess;
    
error_exit:
    cleanupResources();
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
    
    // Unlink VSP parent
    ivars->m_parent = nullptr;
    
    // Remove all IVars resources
    cleanupResources();
    
    /* Shutdown instane */
    if ((ret= Stop(provider, SUPERDISPATCH)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "super::Stop failed. code=%d\n", ret);
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
        VSPLog(LOG_PREFIX, "super::ConnectQueues failed. code=%d\n", ret);
        return ret;
    }
    
    //-- Sane check --//
    
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
    
    // -- Setup RX response queue and dispatch source --
    
    uint64_t size;
    if ((ret = ivars->m_rxqmd->GetLength(&size)) != kIOReturnSuccess || size == 0) {
        VSPLog(LOG_PREFIX, "ConnectQueues: Unable to descriptor size.\n");
        return ret;
    }
 
    // 0 = No options are currently defined.
    // 0 = No priorities are currently defined.
    ret = IODispatchQueue::Create(kRxDataQueueName, 0, 0, &ivars->m_rxQueue);
    if (ret != kIOReturnSuccess || !ivars->m_rxQueue) {
        VSPLog(LOG_PREFIX, "ConnectQueues: Unable to create RX queue. code=%d\n", ret);
        return ret;
    }
    
    ret = IODataQueueDispatchSource::Create(size, ivars->m_rxQueue, &ivars->m_rxSource);
    if (ret != kIOReturnSuccess || !ivars->m_rxSource) {
        VSPLog(LOG_PREFIX, "ConnectQueues: Unable to creade dispatch soure. code=%d\n", ret);
        goto error_exit;
    }

    // Async notification from IODataQueueDispatchSource::DataAvailable
    ret = CreateActionEchoAsyncEvent(size, &ivars->m_rxAction);
    if (ret != kIOReturnSuccess || !ivars->m_rxAction) {
        VSPLog(LOG_PREFIX, "ConnectQueues: Unable to create RX action. code=%d\n", ret);
        goto error_exit;
    }
    
    // Set async async callback action
    ret = ivars->m_rxSource->SetDataAvailableHandler(ivars->m_rxAction);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "ConnectQueues: SetDataAvailableHandler failed. code=%d\n", ret);
        goto error_exit;
    }

    return kIOReturnSuccess;
    
error_exit:
    OSSafeReleaseNULL(ivars->m_rxAction);
    OSSafeReleaseNULL(ivars->m_rxSource);
    OSSafeReleaseNULL(ivars->m_rxQueue);
    return ret;}

// --------------------------------------------------------------------
// DisconnectQueues( ... )
//
kern_return_t IMPL(VSPSerialPort, DisconnectQueues)
{
    IOReturn ret;

    VSPLog(LOG_PREFIX, "DisconnectQueues called\n");

    // reset obtained MD pointers
    ivars->m_txqmd = nullptr;
    ivars->m_rxqmd = nullptr;
    ivars->m_ifmd  = nullptr;

    ret = DisconnectQueues(SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "super::DisconnectQueues: failed. code=%d\n", ret);
        return ret;
    }
    
    return ret;
}

// --------------------------------------------------------------------
// NotifyRXReady_Impl(OSAction* action)
//
void IMPL(VSPSerialPort, NotifyRXReady)
{
    VSPLog(LOG_PREFIX, "NotifyRXReady called.\n");
    
    // Make sure action object is valid
    if (!action) {
        VSPLog(LOG_PREFIX, "NotifyRXReady bad argument. action=0%llx\n", (uint64_t) action);
        return;
    }
    
    // Notify IOSerial rx has data
    RxDataAvailable(SUPERDISPATCH);
    
    // notify tx completion
    if (ivars->m_txIsComplete) {
        ivars->m_txIsComplete = false;
        TxFreeSpaceAvailable(SUPERDISPATCH);
    }
}

// --------------------------------------------------------------------
// EchoAsyncEvent_Impl(OSAction* action)
//
void IMPL(VSPSerialPort, EchoAsyncEvent)
{
    SerialPortInterface* spi;
    IOAddressSegment ifseg;
    IOAddressSegment rxseg;
    IOReturn ret;
    uint64_t deadtime, leeway;
    char* buf;

    VSPLog(LOG_PREFIX, "EchoAsyncEvent called.\n");
    
    // Make sure action object is valid
    if (!action) {
        VSPLog(LOG_PREFIX, "EchoAsyncEvent bad argument. action=0%llx\n", (uint64_t) action);
        return;
    }

    // Lock to ensure thread safety
    IOLockLock(ivars->m_lock);

    if (!ivars->m_rxSource->IsDataAvailable()) {
        goto finished;
    }

    // Lock RX dispatch queue source
    if ((ret = ivars->m_rxSource->SetEnable(false)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "EchoAsyncEvent: RX source SetEnable false failed. code=%d\n", ret);
        goto finished;
    }

    // Get the address of the IOSerialPortInterface
    if ((ret = ivars->m_ifmd->GetAddressRange(&ifseg)) != kIOReturnSuccess) {
       VSPLog(LOG_PREFIX, "EchoAsyncEvent: IF GetAddressRange failed. code=%d\n", ret);
       goto finished;
    }

    // Get the address of the RX ring buffer
    if ((ret = ivars->m_rxqmd->GetAddressRange(&rxseg)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "EchoAsyncEvent: RX GetAddressRange failed. code=%d\n", ret);
        goto finished;
    }
    
    // Pointer to the serial port interface
    spi = (SerialPortInterface*) ifseg.address;
    
    VSPLog(LOG_PREFIX, "EchoAsyncEvent> IOSPI rxPI=%d rxCI=%d rxqoffset=%d rxqlogsz=%d\n",
           spi->rxPI, spi->rxCI, spi->rxqoffset, spi->rxqlogsz);

    // Pointer to the RX ring buffer
    buf = (char*) rxseg.address;

    VSPLog(LOG_PREFIX, "EchoAsyncEvent: dequeue RX source\n");

    // Remove queue entry from RX queue source
    ret = ivars->m_rxSource->Dequeue(^(const void *data, size_t dataSize) {
        VSPLog(LOG_PREFIX, "EchoAsyncEvent: RX dequeue: data=0x%llx size=%ld\n", (uint64_t) data, dataSize);
        // Copy to RX ring buffer
        memcpy(buf, data, dataSize);
        // Update RX producer index
        spi->rxPI = (uint32_t) dataSize;
    });
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "EchoAsyncEvent: RX dequeue failed. code=%d\n", ret);
        switch (ret) {
            case kIOReturnUnderrun:
                VSPLog(LOG_PREFIX, "EchoAsyncEvent: ^^-> underrun\n");
                break;
            case kIOReturnError:
                VSPLog(LOG_PREFIX, "EchoAsyncEvent: ^^-> corrupt\n");
                return;
        }
    }

#ifdef DEBUG // !! Debug ....
    VSPLog(LOG_PREFIX, "EchoAsyncEvent: Dump m_rxqmd -------------\n");
    VSPLog(LOG_PREFIX, "EchoAsyncEvent> buffer=0x%llx size=%u\n", (uint64_t) buf, spi->rxPI);
    for (uint64_t i = 0; i < spi->rxPI; i++) {
        VSPLog(LOG_PREFIX, "EchoAsyncEvent> buffer[%02lld]=0x%02x %c\n", i, buf[i], buf[i]);
    }
#endif

    // Unlock RX queue source
    if ((ret = ivars->m_rxSource->SetEnable(true)) != kIOReturnSuccess) {
       VSPLog(LOG_PREFIX, "EchoAsyncEvent: RX source SetEnable true failed. code=%d\n", ret);
       goto finished;
    }
    
    // Notify queue entry has been removed
    ivars->m_rxSource->SendDataServiced();

    // Notify OS interrest parties
    leeway = 1000000000;
    deadtime = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);
    deadtime += ivars->m_hwLatency;
    ret = ivars->m_tiSource->WakeAtTime(kIOTimerClockMonotonicRaw, deadtime, leeway);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "EchoAsyncEvent> tiSource WakeAtTime failed. code=%d\n", ret);
        goto finished;
    }

    ret = ivars->m_rxSource->Cancel(^{
        VSPLog(LOG_PREFIX, "EchoAsyncEvent> canceled\n");
    });
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "EchoAsyncEvent> rxSource Cancel failed. code=%d\n", ret);
        goto finished;
    }

finished:
    IOLockUnlock(ivars->m_lock);
}

// --------------------------------------------------------------------
// TxDataAvailable_Impl()
//
void IMPL(VSPSerialPort, TxDataAvailable)
{
    SerialPortInterface* spi;
    IOAddressSegment ifseg;
    IOAddressSegment txseg;
    char* buf;
    uint64_t len;
    IOReturn ret;
    
    VSPLog(LOG_PREFIX, "TxDataAvailable called.\n");
    
    // Lock to ensure thread safety
    IOLockLock(ivars->m_lock);

    // Reset first
    ivars->m_txIsComplete = false;

    // Get the address of the IOSerialPortInterface buffer
    if ((ret = ivars->m_ifmd->GetAddressRange(&ifseg)) != kIOReturnSuccess) {
       VSPLog(LOG_PREFIX, "TxDataAvailable: IF GetAddressRange failed. code=%d\n", ret);
       goto finished;
    }

    // Get the address of the TX ring buffer
    if ((ret = ivars->m_txqmd->GetAddressRange(&txseg)) != kIOReturnSuccess) {
       VSPLog(LOG_PREFIX, "TxDataAvailable: TX GetAddressRange failed. code=%d\n", ret);
       goto finished;
    }

    /* serial port interface segment */
    spi = (SerialPortInterface*) ifseg.address;

    VSPLog(LOG_PREFIX, "TxDataAvailable> IOSPI txPI=%d txCI=%d txqoffset=%d txqlogsz=%d\n",
           spi->txPI, spi->txCI, spi->txqoffset, spi->txqlogsz);

    // skip if nothing to do
    if (!spi->txPI) {
        goto finished;
    }
    
    /* txProducer: number bytes comming in */
    len = spi->txPI;
    
    /* Address to the TX ring buffer */
    buf = (char*) txseg.address;

#ifdef DEBUG // !! Debug ....
    VSPLog(LOG_PREFIX, "TxDataAvailable: Dump m_txqmd -------------\n");
    VSPLog(LOG_PREFIX, "TxDataAvailable> buf=0x%llx len=%llu\n", (uint64_t) buf, len);
    for (uint64_t i = 0; i < len; i++) {
        VSPLog(LOG_PREFIX, "TxDataAvailable> buffer[%02lld]=0x%02x %c\n", i, buf[i], buf[i]);
    }
#endif
    
    // Enqueue TX as response (echo)
    if ((ret = enqueueResponse(buf, len, spi)) != kIOReturnSuccess) {
       VSPLog(LOG_PREFIX, "TxDataAvailable: enqueueResponse failed. code=%d\n", ret);
       goto finished;
    }

    // Raise response event
    ivars->m_txIsComplete = true;
    ivars->m_rxSource->SendDataAvailable();
    
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
// SetModemStatus_Impl(bool cts, bool dsr, bool ri, bool dcd)
//
kern_return_t IMPL(VSPSerialPort, SetModemStatus)
{
    IOReturn ret;
    
    VSPLog(LOG_PREFIX, "SetModemStatus called [in] CTS=%d DSR=%d RI=%d DCD=%d\n",
               cts, dsr, ri, dcd);
    
    ret = SetModemStatus(cts, dsr, ri, dcd, SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "super::SetModemStatus failed. code=%d\n", ret);
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
        VSPLog(LOG_PREFIX, "super::RxError: failed. code=%d\n", ret);
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
    
    ivars->m_hwActivated = true;
    
    ret = HwActivate(SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "super::HwActivate failed. code=%d\n", ret);
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
    
    ivars->m_hwActivated = false;
    
    ret = HwDeactivate(SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "super::HwDeactivate failed. code=%d\n", ret);
        return ret;
    }
    
    return ret;
}

// --------------------------------------------------------------------
// HwResetFIFO_Impl()
//
kern_return_t IMPL(VSPSerialPort, HwResetFIFO)
{
    VSPLog(LOG_PREFIX, "HwResetFIFO called -> tx=%d rx=%d\n",
               tx, rx);
    
    // ?? notify caller (IOUserSerial)
    if (rx) {
        ivars->m_hwStatus.cts = true;
        ivars->m_hwStatus.dsr = true;
    }
    
    // ?? notify caller (IOUserSerial)
    if (tx) {
        ivars->m_hwStatus.cts = true;
        ivars->m_hwStatus.dsr = true;
    }
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// HwSendBreak_Impl()
//
kern_return_t IMPL(VSPSerialPort, HwSendBreak)
{
    VSPLog(LOG_PREFIX, "HwSendBreak called -> sendBreak=%d\n", sendBreak);
    
    ivars->m_errorState.gotBreak = sendBreak;
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// HwGetModemStatus_Impl()
//
kern_return_t IMPL(VSPSerialPort, HwGetModemStatus)
{
    VSPLog(LOG_PREFIX, "HwGetModemStatus called [out] CTS=%d DSR=%d RI=%d DCD=%d\n", //
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
    VSPLog(LOG_PREFIX, "HwProgramUART called -> baudRate=%d "
                        "nDataBits=%d nHalfStopBits=%d parity=%d\n",
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
    VSPLog(LOG_PREFIX, "HwProgramBaudRate called -> baudRate=%d\n", baudRate);
    
    ivars->m_uartParams.baudRate = baudRate;
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// HwProgramMCR_Impl()
//
kern_return_t IMPL(VSPSerialPort, HwProgramMCR)
{
    VSPLog(LOG_PREFIX, "HwProgramMCR called -> DTR=%d RTS=%d\n",
               dtr, rts);
    
    ivars->m_hwMCR.dtr = dtr;
    ivars->m_hwMCR.rts = rts;
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// HwProgramLatencyTimer_Impl()
//
kern_return_t IMPL(VSPSerialPort, HwProgramLatencyTimer)
{
    VSPLog(LOG_PREFIX, "HwProgramLatencyTimer called -> latency=%d\n",
               latency);
    
    ivars->m_hwLatency = latency;
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// HwProgramLatencyTimer_Impl()
//
kern_return_t IMPL(VSPSerialPort, HwProgramFlowControl)
{
    VSPLog(LOG_PREFIX, "HwProgramFlowControl called -> arg=%02x xon=%02x xoff=%02x\n",
               arg, xon, xoff);
    
    ivars->m_hwFlowControl.arg = arg;
    ivars->m_hwFlowControl.xon = xon;
    ivars->m_hwFlowControl.xoff = xoff;
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// Remove all resources in IVars
//
void VSPSerialPort::cleanupResources()
{
    VSPLog(LOG_PREFIX, "cleanupResources called.\n");
    
    OSSafeReleaseNULL(ivars->m_tiQueue);
    OSSafeReleaseNULL(ivars->m_tiSource);
    OSSafeReleaseNULL(ivars->m_tiAction);
    OSSafeReleaseNULL(ivars->m_tiData);

    OSSafeReleaseNULL(ivars->m_rxSource);
    OSSafeReleaseNULL(ivars->m_rxAction);
    OSSafeReleaseNULL(ivars->m_rxQueue);
    IOLockFreeNULL(ivars->m_lock);
}

// --------------------------------------------------------------------
//
//
void VSPSerialPort::setParent(VSPDriver* parent)
{
    VSPLog(LOG_PREFIX, "setParent called.\n");

    if (ivars != nullptr) {
        ivars->m_parent = parent;
    }
}

// --------------------------------------------------------------------
//
//
kern_return_t VSPSerialPort::setupTTYBaseName()
{
    IOReturn ret;
    OSDictionary* properties = nullptr;
    OSString* baseName = nullptr;
 
    VSPLog(LOG_PREFIX, "setupTTYBaseName called.\n");

    // setup custom TTY name
    if ((ret = CopyProperties(&properties)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "setupTTYBaseName: Unable to get properties. code=%d\n", ret);
        return ret;
    }
    
    baseName = OSString::withCString(kVSPTTYBaseName);
    properties->setObject(kIOTTYBaseNameKey, baseName);
    
    // write back to driver instance
    if ((ret = SetProperties(properties)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "setupTTYBaseName: Unable to set TTY base name. code=%d\n", ret);
        //return ret; // ??? an error - why???
    }
    
    OSSafeReleaseNULL(baseName);
    OSSafeReleaseNULL(properties);
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// Enque given buffer in RX dispatch source and raise async event
//
kern_return_t VSPSerialPort::enqueueResponse(void* buffer, uint64_t size, void* spif)
{
    IOReturn ret = 0;

    VSPLog(LOG_PREFIX, "enqueueResponse called.\n");

    // Make sure everything is fine
    if (!ivars || !ivars->m_rxSource || !buffer || !size) {
        VSPLog(LOG_PREFIX, "enqueueResponse: Invalid arguments\n");
        return kIOReturnBadArgument;
    }
    
    // Disable dispatching on RX queue source
    if ((ret = ivars->m_rxSource->SetEnable(false)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "enqueueResponse: RX source disable failed. code=%d\n", ret);
        return ret;
    }

    // Response by adding queue entry to RX queue source
    ret = ivars->m_rxSource->Enqueue((uint32_t) size, ^(void *data, size_t dataSize) {
        VSPLog(LOG_PREFIX, "enqueueResponse: RX enqueue data=0x%llx size=%lld\n",
               (uint64_t) data, size);
        memcpy(data, buffer, (dataSize < size ? dataSize : size));
    });
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "enqueueResponse: RX enqueue failed. code=%d\n", ret);
        switch (ret) {
            case kIOReturnOverrun:
                VSPLog(LOG_PREFIX, "enqueueResponse: ^^-> overrun\n");
                break;
            case kIOReturnError:
                VSPLog(LOG_PREFIX, "enqueueResponse: ^^-> corrupt\n");
                break;
        }
        // Leave dispatch disabled (??)
        return ret;
    }
    
    // Enable dispatching on RX queue source
    if ((ret = ivars->m_rxSource->SetEnable(true)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "enqueueResponse: RX source enable failed. code=%d\n", ret);
        return ret;
    }
    
    return kIOReturnSuccess;
}
