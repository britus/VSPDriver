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
using namespace driverkit::serial;

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

#define VSPAquireLock(ivars) \
{ \
    ++ivars->m_lockLevel; \
    VSPLog(LOG_PREFIX, "=> lock level=%d", ivars->m_lockLevel); \
    IOLockLock(ivars->m_lock); \
}

#define VSPUnlock(ivars) \
{ \
    VSPLog(LOG_PREFIX, "<= lock level=%d", ivars->m_lockLevel); \
    --ivars->m_lockLevel; \
    IOLockUnlock(ivars->m_lock); \
}

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
    volatile atomic_int m_lockLevel = 0;

    /* OS provided memory descriptors by overridden
     * method ConnectQueues(...) */
    SerialPortInterface* m_spi;                     // OS serial port interface
    IOAddressSegment m_txseg = {};                  // OS TX buffer segment
    IOAddressSegment m_rxseg = {};                  // OS RX buffer segment
    
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
    uint32_t m_hwLatency = 25;
    
    bool m_txIsComplete = false;
    bool m_rxIsComplete = false;
    bool m_hwActivated = false;
};

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
// Remove all resources in IVars
//
void VSPSerialPort::cleanupResources()
{
    VSPLog(LOG_PREFIX, "cleanupResources called.\n");
    
    OSSafeReleaseNULL(ivars->m_tiQueue);
    OSSafeReleaseNULL(ivars->m_tiSource);
    OSSafeReleaseNULL(ivars->m_tiAction);
    OSSafeReleaseNULL(ivars->m_tiData);
    IOLockFreeNULL(ivars->m_lock);
}

// ====================================================================
// ** ----------------[ Connection live cycle ]--------------------- **
// ====================================================================

// --------------------------------------------------------------------
// ConnectQueues_Impl( ... )
// First call
kern_return_t IMPL(VSPSerialPort, ConnectQueues)
{
    // SerialPortInterface related
    IOBufferMemoryDescriptor* ifbmd = nullptr;
    IOBufferMemoryDescriptor* txqbmd = nullptr;
    IOBufferMemoryDescriptor* rxqbmd = nullptr;

    IOAddressSegment ifseg = {};

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
    if (!ifmd || !(*ifmd) || !txqmd || !(*txqmd) || !rxqmd || !(*rxqmd)) {
        VSPLog(LOG_PREFIX, "ConnectQueues: Invalid memory descriptors detected. (NULL)\n");
        return kIOReturnBadArgument;
    }
    
    ifbmd = OSDynamicCast(IOBufferMemoryDescriptor, (*ifmd));
    if (ifbmd == nullptr) {
        VSPLog(LOG_PREFIX, "ConnectQueues: Invalid 'ifmd' memory descriptor detected.\n");
        return kIOReturnInvalid;
    }
    
    txqbmd = OSDynamicCast(IOBufferMemoryDescriptor, (*txqmd));
    if (txqbmd == nullptr) {
        VSPLog(LOG_PREFIX, "ConnectQueues: Invalid 'txqmd' memory descriptor detected.\n");
        return kIOReturnInvalid;
    }
    
    rxqbmd = OSDynamicCast(IOBufferMemoryDescriptor, (*rxqmd));
    if (rxqbmd == nullptr) {
        VSPLog(LOG_PREFIX, "ConnectQueues: Invalid 'rxqmd' memory descriptor detected.\n");
        return kIOReturnInvalid;
    }
    
    // Get the address of the IOSerialPortInterface segment (mapped space)
    if ((ret = ifbmd->GetAddressRange(&ifseg)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "ConnectQueues: IF GetAddressRange failed. code=%d\n", ret);
        return ret;
    }
    ivars->m_spi = reinterpret_cast<SerialPortInterface*>(ifseg.address);
   
    // Initialize the indexes
    ivars->m_spi->txCI = 0;
    ivars->m_spi->txPI = 0;
    
    // Get the address of the TX ring buffer segment (mapped space)
    if ((ret = txqbmd->GetAddressRange(&ivars->m_txseg)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "ConnectQueues: TX GetAddressRange failed. code=%d\n", ret);
        return ret;
    }

    // Get the address of the RX ring buffer segment (mapped space)
    if ((ret = rxqbmd->GetAddressRange(&ivars->m_rxseg)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "ConnectQueues: TX GetAddressRange failed. code=%d\n", ret);
        return ret;
    }

    // -- Setup RX response queue and dispatch source --
    
    uint64_t size;
    if ((ret = rxqbmd->GetLength(&size)) != kIOReturnSuccess || size == 0) {
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
    ret = CreateActionRxEchoAsyncEvent(size, &ivars->m_rxAction);
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

    // ???
    ivars->m_hwStatus.dcd = true;
    // ???
    ivars->m_hwStatus.cts = true;

    return kIOReturnSuccess;
    
error_exit:
    OSSafeReleaseNULL(ivars->m_rxAction);
    OSSafeReleaseNULL(ivars->m_rxSource);
    OSSafeReleaseNULL(ivars->m_rxQueue);
    return ret;
}

// --------------------------------------------------------------------
// DisconnectQueues_Impl()
// Last call
kern_return_t IMPL(VSPSerialPort, DisconnectQueues)
{
    IOReturn ret;

    VSPLog(LOG_PREFIX, "DisconnectQueues called\n");
    
    // Lock to ensure thread safety
    VSPAquireLock(ivars);

    // stop RX dispatch queue
    OSSafeReleaseNULL(ivars->m_rxSource);
    OSSafeReleaseNULL(ivars->m_rxAction);
    OSSafeReleaseNULL(ivars->m_rxQueue);
  
    // ???
    ivars->m_hwStatus.dcd = false;
    ivars->m_hwStatus.cts = false;

    // reset spi pointer and RX/TX buffer segments
    ivars->m_spi   = nullptr;
    ivars->m_rxseg = {};
    ivars->m_txseg = {};

    // Unlock thread
    VSPUnlock(ivars);

    ret = DisconnectQueues(SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "super::DisconnectQueues: failed. code=%d\n", ret);
        return ret;
    }
    
    return ret;
}

// --------------------------------------------------------------------
// NotifyRXReady_Impl(OSAction* action)
// Called by timer dispatch queue source
void IMPL(VSPSerialPort, NotifyRXReady)
{
    VSPLog(LOG_PREFIX, "NotifyRXReady called.\n");
    
    // Make sure action object is valid
    if (!action) {
        VSPLog(LOG_PREFIX, "NotifyRXReady bad argument. action=0%llx\n", (uint64_t) action);
        return;
    }
    
    // Notify IOUserSerial rx data ready to dispatch
    this->RxDataAvailable_Impl();
}

// --------------------------------------------------------------------
// RxEchoAsyncEvent_Impl(OSAction* action)
// Called by RX data available dispatch queue source
void IMPL(VSPSerialPort, RxEchoAsyncEvent)
{
    IOReturn ret;
    uint8_t* buf;

    // Lock to ensure thread safety
    VSPAquireLock(ivars);

    // skip if complete...
    // RxEchoAsyncEvent is called twice by OS
    if (!ivars->m_rxSource->IsDataAvailable()) {
        //VSPLog(LOG_PREFIX, "RxEchoAsyncEvent RX source is empty, done.\n");
        goto finish;
    }

    VSPLog(LOG_PREFIX, "++++++++++++++++++++++++++++++++++++++++\n");
    VSPLog(LOG_PREFIX, "RxEchoAsyncEvent: called +++++++++++++++\n");

    // Requiered: Reset OS consumed RX index.
    ivars->m_spi->rxCI = 0;
    
    // Make sure nothing is left
    ivars->m_spi->rxPI = 0;
    ivars->m_hwStatus.cts = false;
    ivars->m_hwStatus.dsr = false;

    // Make sure action object is valid
    if (!action) {
        VSPLog(LOG_PREFIX, "RxEchoAsyncEvent bad argument. action=0%llx\n", (uint64_t) action);
        goto finish;
    }

    // Lock RX dispatch queue source
    if ((ret = ivars->m_rxSource->SetEnable(false)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "RxEchoAsyncEvent: RX source SetEnable false failed. code=%d\n", ret);
        goto finish;
    }

    // RX address to the RX ring buffer
    buf = (uint8_t*) ivars->m_rxseg.address;

    VSPLog(LOG_PREFIX, "RxEchoAsyncEvent: dequeue RX source\n");

    // Remove queue entry from RX queue source
    ret = ivars->m_rxSource->Dequeue(^(const void *data, size_t dataSize) {
        VSPLog(LOG_PREFIX, "RxEchoAsyncEvent: RX dequeue: data=0x%llx size=%ld\n", (uint64_t) data, dataSize);
        // Copy data from RX queue source to RX-MD buffer
        for (uint64_t i = 0; i < dataSize; i++) {
            buf[i] = ((uint8_t*)data)[i];
        }
        // Update RX producer index
        ivars->m_spi->rxPI = (uint32_t) dataSize;
    });
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "RxEchoAsyncEvent: RX dequeue failed. code=%d\n", ret);
        switch (ret) {
            case kIOReturnUnderrun:
                VSPLog(LOG_PREFIX, "RxEchoAsyncEvent: ^^-> underrun\n");
                break;
            case kIOReturnError:
                VSPLog(LOG_PREFIX, "RxEchoAsyncEvent: ^^-> corrupt\n");
                break;
        }
        goto finish;
    }
    
    // Notify queue entry has been removed
    ivars->m_rxSource->SendDataServiced();

    VSPLog(LOG_PREFIX, "RxEchoAsyncEvent> IOSPI rxPI=%d rxCI=%d rxqoffset=%d rxqlogsz=%d\n",
           ivars->m_spi->rxPI, ivars->m_spi->rxCI, ivars->m_spi->rxqoffset, ivars->m_spi->rxqlogsz);

#ifdef DEBUG // !! Debug ....
    VSPLog(LOG_PREFIX, "RxEchoAsyncEvent: Dump m_rxqmd buffer=0x%llx size=%u\n", (uint64_t) buf, ivars->m_spi->rxPI);
    for (uint64_t i = 0; i < ivars->m_spi->rxPI; i++) {
       VSPLog(LOG_PREFIX, "RxEchoAsyncEvent> buffer[%02lld]=0x%02x %c\n", i, buf[i], buf[i]);
    }
#endif
        
    // ???
    ivars->m_hwStatus.cts = true;
    ivars->m_hwStatus.dsr = true;

    // Unlock RX queue source
    if ((ret = ivars->m_rxSource->SetEnable(true)) != kIOReturnSuccess) {
       VSPLog(LOG_PREFIX, "RxEchoAsyncEvent: RX source SetEnable true failed. code=%d\n", ret);
       goto finish;
    }

    // Notify RX complete to OS interrest parties
    this->RxDataAvailable_Impl();
        
    VSPLog(LOG_PREFIX, "RxEchoAsyncEvent: complete\n");
    VSPLog(LOG_PREFIX, "++++++++++++++++++++++++++++++++++++++++\n");
    
finish:
    VSPUnlock(ivars);
}

// --------------------------------------------------------------------
// TxDataAvailable_Impl()
// Called on TX data income
void IMPL(VSPSerialPort, TxDataAvailable)
{
    size_t tx_buf_sz;
    void* src;
    void* dest;
    
    VSPLog(LOG_PREFIX, "---------------------------------------------\n");
    VSPLog(LOG_PREFIX, "TxDataAvailable called.\n");
    
    // Lock to ensure thread safety
    VSPAquireLock(ivars);

    // Reset first
    ivars->m_txIsComplete = false;
    
    // We working...
    ivars->m_hwStatus.cts = false;
    ivars->m_hwStatus.dsr = false;

    // Show me indexes be fore manipulate
    VSPLog(LOG_PREFIX, "TxDataAvailable: [IOSPI-TX 1] txPI: %d, txCI: %d, rxPI: %d, rxCI: %d",
           ivars->m_spi->txPI, ivars->m_spi->txCI,
           ivars->m_spi->rxPI, ivars->m_spi->rxCI);

    // skip if nothing to do
    if (!ivars->m_spi->txPI) {
        VSPLog(LOG_PREFIX, "TxDataAvailable: spi->txPI is zero, skip\n");
        goto finish;
    }

    // Berechnung der verfügbaren Daten im TX-Puffer
    tx_buf_sz = ivars->m_spi->txPI - ivars->m_spi->txCI;
    
    src = reinterpret_cast<void*>(ivars->m_txseg.address + ivars->m_spi->txCI);
    dest = reinterpret_cast<void*>(ivars->m_rxseg.address + ivars->m_spi->rxPI);

    VSPLog(LOG_PREFIX, "TxDataAvailable: copy TX->RX src=0x%llx dst=%llx len=%ld\n",
           (uint64_t) src, (uint64_t) dest, tx_buf_sz);

    // Kopieren der Daten vom TX- zum RX-Puffer (Loopback-Beispiel)
    memcpy(dest, src, tx_buf_sz);
    
    // Zurücksetzen des TX-Consumer-Index
    ivars->m_spi->txCI = ivars->m_spi->txPI;
    
    // Aktualisierung des Producer-Index für RX
    ivars->m_spi->rxPI += tx_buf_sz;

    // Show me indexes be fore manipulation
    VSPLog(LOG_PREFIX, "TxDataAvailable: [IOSPI-TX 2] txPI: %d, txCI: %d, rxPI: %d, rxCI: %d",
           ivars->m_spi->txPI, ivars->m_spi->txCI,
           ivars->m_spi->rxPI, ivars->m_spi->rxCI);

#ifdef DEBUG // !! Debug ....
    for (uint64_t i = 0; i < tx_buf_sz; i++) {
        VSPLog(LOG_PREFIX, "TxDataAvailable: dest[%02lld]=0x%02x %c\n",
               i, ((char*)dest)[i], ((char*)dest)[i]);
    }
#endif

    // TX -> RX echo done
    ivars->m_txIsComplete = true;

    VSPLog(LOG_PREFIX, "TxDataAvailable complete.\n");
    VSPLog(LOG_PREFIX, "---------------------------------------------\n");
    
    // Notify about
    ivars->m_hwStatus.cts = true;
    ivars->m_hwStatus.dsr = true;
    this->RxDataAvailable_Impl();
finish:
    VSPUnlock(ivars);
}

// --------------------------------------------------------------------
// Enque given buffer in RX dispatch source and raise async event
// Enqueue given buffer into RX dispatch queue source
kern_return_t VSPSerialPort::enqueueResponse(void* buffer, uint64_t size)
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
        VSPLog(LOG_PREFIX, "enqueueResponse: RX enqueue data=0x%llx size=%lld\n", (uint64_t) data, size);
        if (dataSize < size) {
            memset(data, 0xff, dataSize);
            return;
        }
        memset(data, 0,      size);
        memcpy(data, buffer, size);
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

// --------------------------------------------------------------------
// RxDataAvailable_Impl()
// Notify OS response RX data ready for client
void IMPL(VSPSerialPort, RxDataAvailable)
{
    VSPLog(LOG_PREFIX, "RxDataAvailable called.\n");
    RxDataAvailable(SUPERDISPATCH);
}

// --------------------------------------------------------------------
// RxFreeSpaceAvailable_Impl()
// Notification to this instance that RX buffer space is available for
// your device’s data
void IMPL(VSPSerialPort, RxFreeSpaceAvailable)
{
    VSPLog(LOG_PREFIX, "RxFreeSpaceAvailable called.\n");

    VSPAquireLock(ivars);
    
    VSPLog(LOG_PREFIX, "RxFreeSpaceAvailable> IOSPI rxPI=%d rxCI=%d rxqoffset=%d rxqlogsz=%d\n",
           ivars->m_spi->rxPI, ivars->m_spi->rxCI, ivars->m_spi->rxqoffset, ivars->m_spi->rxqlogsz);
    VSPLog(LOG_PREFIX, "RxFreeSpaceAvailable> IOSPI txPI=%d txCI=%d txqoffset=%d txqlogsz=%d\n",
           ivars->m_spi->txPI, ivars->m_spi->txCI, ivars->m_spi->txqoffset, ivars->m_spi->txqlogsz);

    VSPUnlock(ivars);

    RxFreeSpaceAvailable(SUPERDISPATCH);
}

// --------------------------------------------------------------------
// TxFreeSpaceAvailable_Impl()
// Notify OS ready for more client data
void IMPL(VSPSerialPort, TxFreeSpaceAvailable)
{
    VSPLog(LOG_PREFIX, "TxFreeSpaceAvailable called.\n");

    VSPAquireLock(ivars);
    
    VSPLog(LOG_PREFIX, "RxFreeSpaceAvailable> IOSPI rxPI=%d rxCI=%d rxqoffset=%d rxqlogsz=%d\n",
           ivars->m_spi->rxPI, ivars->m_spi->rxCI, ivars->m_spi->rxqoffset, ivars->m_spi->rxqlogsz);
    VSPLog(LOG_PREFIX, "RxFreeSpaceAvailable> IOSPI txPI=%d txCI=%d txqoffset=%d txqlogsz=%d\n",
           ivars->m_spi->txPI, ivars->m_spi->txCI, ivars->m_spi->txqoffset, ivars->m_spi->txqlogsz);

    VSPUnlock(ivars);

    TxFreeSpaceAvailable(SUPERDISPATCH);
}

// --------------------------------------------------------------------
// SetModemStatus_Impl(bool cts, bool dsr, bool ri, bool dcd)
// Called during serial port setup or communication
kern_return_t IMPL(VSPSerialPort, SetModemStatus)
{
    IOReturn ret;
    
    VSPLog(LOG_PREFIX, "SetModemStatus called [in] CTS=%d DSR=%d RI=%d DCD=%d\n",
               cts, dsr, ri, dcd);
    
    VSPAquireLock(ivars);
    ivars->m_hwStatus.cts = cts;
    ivars->m_hwStatus.dsr = dsr;
    ivars->m_hwStatus.ri  = ri;
    ivars->m_hwStatus.dcd = dcd;
    VSPUnlock(ivars);
    
    ret = SetModemStatus(cts, dsr, ri, dcd, SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "super::SetModemStatus failed. code=%d\n", ret);
        return ret;
    }

    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// RxError_Impl(bool overrun, bool break, bool framing, bool parity)
// Called on given error states
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
    
    VSPAquireLock(ivars);
    ivars->m_errorState.overrun = overrun;
    ivars->m_errorState.framingError = framingError;
    ivars->m_errorState.gotBreak = gotBreak;
    ivars->m_errorState.parityError = parityError;
    VSPUnlock(ivars);
    
    ret = RxError(overrun, gotBreak, framingError, parityError, SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "super::RxError: failed. code=%d\n", ret);
        return ret;
    }

    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// HwActivate_Impl()
// Called after ConnectQueues() or other reasons
kern_return_t IMPL(VSPSerialPort, HwActivate)
{
    kern_return_t ret = kIOReturnIOError;
    
    VSPLog(LOG_PREFIX, "HwActivate called.\n");
    
    VSPAquireLock(ivars);
    ivars->m_hwActivated = true;
    // ???
    ivars->m_hwStatus.dcd = true;
    // ???
    ivars->m_hwStatus.cts = true;
    VSPUnlock(ivars);
    
    ret = HwActivate(SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "super::HwActivate failed. code=%d\n", ret);
        return ret;
    }
    
    return ret;
}

// --------------------------------------------------------------------
// HwDeactivate_Impl()
// Called before DisconnectQueues() or other reasons
kern_return_t IMPL(VSPSerialPort, HwDeactivate)
{
    kern_return_t ret = kIOReturnIOError;
    
    VSPLog(LOG_PREFIX, "HwDeactivate called.\n");
    
    VSPAquireLock(ivars);
    ivars->m_hwActivated = false;
    // ???
    ivars->m_hwStatus.dcd = false;
    // ???
    ivars->m_hwStatus.cts = false;
    VSPUnlock(ivars);
    
    ret = HwDeactivate(SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "super::HwDeactivate failed. code=%d\n", ret);
        return ret;
    }
    
    return ret;
}

// --------------------------------------------------------------------
// HwResetFIFO_Impl()
// Called by client to TxFreeSpaceAvailable or RxFreeSpaceAvailable
// or other reasons.
kern_return_t IMPL(VSPSerialPort, HwResetFIFO)
{
    VSPLog(LOG_PREFIX, "HwResetFIFO called -> tx=%d rx=%d\n",
               tx, rx);
    
    VSPAquireLock(ivars);
    // ?? notify caller (IOUserSerial)
    if (rx) {
        ivars->m_hwStatus.dsr = false;
    }
    
    // ?? notify caller (IOUserSerial)
    if (tx) {
        ivars->m_hwStatus.cts = true;
    }
    VSPUnlock(ivars);
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// HwSendBreak_Impl()
// Called during client communication life cycle
kern_return_t IMPL(VSPSerialPort, HwSendBreak)
{
    VSPLog(LOG_PREFIX, "HwSendBreak called -> sendBreak=%d\n", sendBreak);
    
    VSPAquireLock(ivars);
    ivars->m_errorState.gotBreak = sendBreak;
    VSPUnlock(ivars);
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// HwGetModemStatus_Impl()
// Called during client communication life cycle
kern_return_t IMPL(VSPSerialPort, HwGetModemStatus)
{
    VSPLog(LOG_PREFIX, "HwGetModemStatus called [out] CTS=%d DSR=%d RI=%d DCD=%d\n", //
               ivars->m_hwStatus.cts, ivars->m_hwStatus.dsr, //
               ivars->m_hwStatus.ri, ivars->m_hwStatus.dcd);
    
    VSPAquireLock(ivars);
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
    VSPUnlock(ivars);
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// HwProgramUART_Impl()
// Called during serial port setup or other reason
kern_return_t IMPL(VSPSerialPort, HwProgramUART)
{
    VSPLog(LOG_PREFIX, "HwProgramUART called -> baudRate=%d "
                        "nDataBits=%d nHalfStopBits=%d parity=%d\n",
            baudRate, nDataBits, nHalfStopBits, parity);
    
    VSPAquireLock(ivars);
    ivars->m_uartParams.baudRate = baudRate;
    ivars->m_uartParams.nDataBits = nDataBits;
    ivars->m_uartParams.nHalfStopBits = nHalfStopBits;
    ivars->m_uartParams.parity = parity;
    VSPUnlock(ivars);
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// HwProgramBaudRate_Impl()
// Called during serial port setup or other reason
kern_return_t IMPL(VSPSerialPort, HwProgramBaudRate)
{
    VSPLog(LOG_PREFIX, "HwProgramBaudRate called -> baudRate=%d\n", baudRate);
    
    VSPAquireLock(ivars);
    ivars->m_uartParams.baudRate = baudRate;
    VSPUnlock(ivars);
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// HwProgramMCR_Impl()
// Called during serial port setup or other reason
kern_return_t IMPL(VSPSerialPort, HwProgramMCR)
{
    VSPLog(LOG_PREFIX, "HwProgramMCR called -> DTR=%d RTS=%d\n",
               dtr, rts);
    
    VSPAquireLock(ivars);
    ivars->m_hwMCR.dtr = dtr;
    ivars->m_hwMCR.rts = rts;
    VSPUnlock(ivars);
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// HwProgramLatencyTimer_Impl()
// Called during serial port setup or other reason
kern_return_t IMPL(VSPSerialPort, HwProgramLatencyTimer)
{
    VSPLog(LOG_PREFIX, "HwProgramLatencyTimer called -> latency=%d\n",
               latency);
    
    VSPAquireLock(ivars);
    ivars->m_hwLatency = latency;
    VSPUnlock(ivars);
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// HwProgramLatencyTimer_Impl()
// Called during serial port setup or other reason
kern_return_t IMPL(VSPSerialPort, HwProgramFlowControl)
{
    VSPLog(LOG_PREFIX, "HwProgramFlowControl called -> arg=%02x xon=%02x xoff=%02x\n",
               arg, xon, xoff);
    
    VSPAquireLock(ivars);
    ivars->m_hwFlowControl.arg = arg;
    ivars->m_hwFlowControl.xon = xon;
    ivars->m_hwFlowControl.xoff = xoff;
    VSPUnlock(ivars);
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// Called by VSPDriver instance to link to parent level
//
void VSPSerialPort::setParent(VSPDriver* parent)
{
    VSPLog(LOG_PREFIX, "setParent called.\n");

    if (ivars != nullptr && !ivars->m_parent) {
        ivars->m_parent = parent;
    }
}

// --------------------------------------------------------------------
// Called by VSPDriver instance to set TTY base and number based on
// managed instance of this object instance
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
   
    //CreateNameMatchingDictionary(<#OSString *serviceName#>, <#OSDictionary *matching#>)
    //baseName = OSString::withCString(kVSPTTYBaseName);
    //properties->setObject(kIOTTYBaseNameKey, baseName);
    
    // write back to driver instance
    if ((ret = SetProperties(properties)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "setupTTYBaseName: Unable to set TTY base name. code=%d\n", ret);
        //return ret; // ??? an error - why???
    }
    
    OSSafeReleaseNULL(baseName);
    OSSafeReleaseNULL(properties);
    return kIOReturnSuccess;
}



// Notify RX complete to OS interrest parties
// using client defined latency time
//ret = ivars->m_tiSource->WakeAtTime(
//                                    kIOTimerClockMonotonicRaw,
//                                    clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW)
//                                    + (ivars->m_hwLatency * 1000000),
//                                    1000000000);
//if (ret != kIOReturnSuccess) {
//    VSPLog(LOG_PREFIX, "RxEchoAsyncEvent> tiSource WakeAtTime failed. code=%d\n", ret);
//    goto finish;
//}
