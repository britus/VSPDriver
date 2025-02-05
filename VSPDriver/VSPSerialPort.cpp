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
#define SERVER_ADDRESS "127.0.0.1"
#define SERVER_PORT 9001

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
    IOBufferMemoryDescriptor *m_ifmd = nullptr;     // Interrupt related buffer
    IOMemoryDescriptor *m_txqmd = nullptr;          // Transmit buffer
    IOMemoryDescriptor *m_rxqmd = nullptr;          // Receive buffer
    OSData* m_txOSData = nullptr;                   // ?? for ConfigureReport
    OSData* m_rxOSData = nullptr;                   // ?? for ConfigureReport
    
    IODispatchQueue* m_dataQueue = nullptr;
    IODataQueueDispatchSource* m_dataSource = nullptr;
    OSAction* m_dataAction = nullptr;               // Async get packets action
    
    VSPDriver* m_parent;
    
    IOLock* m_lock = nullptr;
    
    // Serial interface
    TErrorState m_errorState = {};
    TUartParameters m_uartParams = {};
    THwSerialStatus m_hwStatus = {};
    THwFlowControl m_hwFlowControl = {};
    THwMCR m_hwMCR = {};
    uint32_t m_hwLatency;
    
    // TCP socket connection details
    OSString *m_serverAddress = nullptr;
    uint16_t m_serverPort = 0;
    bool m_isConnected = false;
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
    
    /* check our private driver instance */
    if (!ivars) {
        VSPLog(LOG_PREFIX, "Start: Private driver instance is NULL\n");
        return kIOReturnInvalid;
    }

#if 0
    // Check caller object type VSPDriver
    ivars->m_parent = OSDynamicCast(VSPDriver, provider);
    if (ivars->m_parent == nullptr) {
        VSPLog(LOG_PREFIX, "Start: Cast to VSPDriver failed.\n");
        return kIOReturnBadArgument;
    }
#endif
    
    /* call apple style super method */
    ret = Start(provider, SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "Start(super): failed. code=%d\n", ret);
        return ret;
    }
    
    // the locker
    ivars->m_lock = IOLockAlloc();
    if (ivars->m_lock == nullptr) {
        VSPLog(LOG_PREFIX, "Start: Unable to allocate lock object.\n");
        goto error_exit;
    }
    
    // remember OS provider
    ivars->m_provider = provider;
    
    // default UART parameters
    ivars->m_uartParams.baudRate = 112500;
    ivars->m_uartParams.nHalfStopBits = 1;
    ivars->m_uartParams.nDataBits = 8;
    ivars->m_uartParams.parity = 0;
    
    VSPLog(LOG_PREFIX, "Start: Connect INT/RX/TX buffers.\n");
    
    // connect dispatcher queues and get its memory descriptors
    if ((ret = ConnectDriverQueues()) != kIOReturnSuccess) {
        goto error_exit;
    }
    
    VSPLog(LOG_PREFIX, "Start: Get memory descriptor size.\n");
    
    uint64_t mdSize;
    if ((ret = ivars->m_txqmd->GetLength(&mdSize)) != kIOReturnSuccess || mdSize == 0) {
        VSPLog(LOG_PREFIX, "Start: Unable to descriptor size.\n");
        goto error_exit;
    }
    
    VSPLog(LOG_PREFIX, "Start: Connect IODataQueueDispatchSource::DataAvailable event.\n");
    
    IODispatchQueueName name;
    strncpy(name, "vcpDataQueue", sizeof(IODispatchQueueName)-1);
    
    // 0 = No options are currently defined.
    // 0 = No priorities are currently defined.
    ret = IODispatchQueue::Create(name, 0, 0, &ivars->m_dataQueue);
    if (ret != kIOReturnSuccess || ivars->m_dataQueue == nullptr) {
        VSPLog(LOG_PREFIX, "Start: Unable to create ifQueue. code=%d\n", ret);
        goto error_exit;
    }
    
    ret = IODataQueueDispatchSource::Create(mdSize, ivars->m_dataQueue, &ivars->m_dataSource);
    if (ret != kIOReturnSuccess || ivars->m_dataSource == nullptr) {
        VSPLog(LOG_PREFIX, "Start: IODataQueueDispatchSource::Create failed. code=%d\n", ret);
        goto error_exit;
    }
    
    // Async notification from IODataQueueDispatchSource::DataAvailable
    ret = CreateActionTxPacketsAvailable(mdSize, &ivars->m_dataAction);
    if (ret != kIOReturnSuccess || ivars->m_dataAction == nullptr) {
        VSPLog(LOG_PREFIX, "Start: Unable to create TX packet action. code=%d\n", ret);
        goto error_exit;
    }
    
    ret = ivars->m_dataSource->SetDataAvailableHandler(ivars->m_dataAction);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "Start: m_txDataQDSource->SetDataAvailableHandler failed. code=%d\n", ret);
        goto error_exit;
    }
    
    if ((ret = SetupTTYBaseName()) != kIOReturnSuccess) {
        goto error_exit;
    }
    
    VSPLog(LOG_PREFIX, "Start: prepare internal stuff.\n");
    
    // Default TCP server settings
    ivars->m_serverPort = SERVER_PORT;
    ivars->m_serverAddress = OSString::withCString(SERVER_ADDRESS);
    if (ivars->m_serverAddress == nullptr) {
        ret = kIOReturnNoMemory;
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
    
    /* remove all IVars resources */
    CleanupResources();
    
    /* shutdown */
    if ((ret= Stop(provider, SUPERDISPATCH)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "Stop (suprt) failed. code=%d\n", ret);
    } else {
        VSPLog(LOG_PREFIX, "driver successfully removed.\n");
    }
    
    return ret;
}

// --------------------------------------------------------------------
// RxDataAvailable_Impl()
//
void IMPL(VSPSerialPort, RxDataAvailable)
{
    VSPLog(LOG_PREFIX, "RxDataAvailable called.\n");
    
    
    RxDataAvailable(SUPERDISPATCH);
}

// --------------------------------------------------------------------
// TxDataAvailable_Impl()
//
void IMPL(VSPSerialPort, TxDataAvailable)
{
    IOReturn ret = kIOReturnSuccess;
    
    VSPLog(LOG_PREFIX, "TxDataAvailable called.\n");
    
    // Lock to ensure thread safety
    IOLockLock(ivars->m_lock);
    
    VSPLog(LOG_PREFIX, "TxDataAvailable: Dump m_txqmd -------------\n");
    ret = CopyMemory(ivars->m_txqmd);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "TxDataAvailable: CopyMemory failed on m_txqmd\n");
        goto finished;
    }
    
    // next data from /dev/cu.serial-xxxxxxx device
    TxFreeSpaceAvailable();
    
    // --
    RxDataAvailable();
finished:
    IOLockUnlock(ivars->m_lock);
}

// --------------------------------------------------------------------
// TxPacketsAvailable_Impl(OSAction* action) Async callback handler
//
void IMPL(VSPSerialPort, TxPacketsAvailable)
{
    VSPLog(LOG_PREFIX, "TxPacketsAvailable called.\n");
    
    if (action == nullptr) {
        VSPLog(LOG_PREFIX, "TxPacketsAvailable: Invalid action object (nullptr).\n");
        return;
    }
    
    // ****
    //if (ivars->m_txDataQDSource->IsDataAvailable()) {
    //    ivars->m_txDataQDSource->SendDataAvailable();
    //}
    
    // Lock to ensure thread safety
    IOLockLock(ivars->m_lock);
    /*
     const uint64_t size = ivars->m_fifo.tx.size;
     const char* buffer = (char*)action->GetReference();
     if (buffer == nullptr) {
     VSPLog(LOG_PREFIX, "TxPacketsAvailable: Invalid buffer pointer (nullptr).\n");
     goto finished;
     }
     
 // !! Debug ....
     for (uint64_t i = 0; i < size && i < 16; i++) {
     VSPLog(LOG_PREFIX, "TxPacketsAvailable: TX> 0x%02x %c\n", buffer[i], buffer[i]);
     }
     */
    
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
// CleanupResources_Impl() Remove all resources
//
void IMPL(VSPSerialPort, CleanupResources)
{
    VSPLog(LOG_PREFIX, "CleanupResources called.\n");
    
    OSSafeReleaseNULL(ivars->m_txOSData);
    OSSafeReleaseNULL(ivars->m_rxOSData);
    
    OSSafeReleaseNULL(ivars->m_dataSource);
    OSSafeReleaseNULL(ivars->m_dataQueue);
    OSSafeReleaseNULL(ivars->m_dataAction);
    
    OSSafeReleaseNULL(ivars->m_serverAddress);
    
    // Disconnect all queues. This deallocates
    // the INT/RX/TX buffer resources too
    this->DisconnectQueues();
    
    IOLockFreeNULL(ivars->m_lock);
}

// --------------------------------------------------------------------
// SetupTTYBaseName_Impl()
//
IOReturn IMPL(VSPSerialPort, SetupTTYBaseName)
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

// --------------------------------------------------------------------
// SetupTTYBaseName_Impl()
//
IOReturn IMPL(VSPSerialPort, ConnectDriverQueues)
{
    IOReturn ret;
    
    ret = this->ConnectQueues(&ivars->m_ifmd,   // --
                              &ivars->m_rxqmd,  // --
                              &ivars->m_txqmd,  // --
                              nullptr, nullptr, 0, 0, 8, 8);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "ConnectQueues failed to allocate IF/RX/TX buffers.\n");
        return ret;
    }
    if (ivars->m_ifmd == nullptr) {
        VSPLog(LOG_PREFIX, "ConnectQueues: Invalid interrupt buffer detected.\n");
        return kIOReturnInvalid;
    }
    if (ivars->m_rxqmd == nullptr) {
        VSPLog(LOG_PREFIX, "ConnectQueues: Invalid RX buffer detected.\n");
        return kIOReturnInvalid;
    }
    if (ivars->m_txqmd == nullptr) {
        VSPLog(LOG_PREFIX, "ConnectQueues: Invalid TX buffer detected.\n");
        return kIOReturnInvalid;
    }
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// CopyMemory_Impl(IOMemoryDescriptor* md)
// ??? Called by TxDataAvailable() and here we get always 0x00 in MD
// ??? mapped buffer of the IOMemoryDescriptors m_txqmd
//
IOReturn IMPL(VSPSerialPort, CopyMemory)
{
    IOMemoryMap* map = nullptr;
    IOReturn ret;
    
    VSPLog(LOG_PREFIX, "CopyMemory called. md=0x%llx\n", (uint64_t)md);
    
    if (md == nullptr) {
        VSPLog(LOG_PREFIX, "copy_md_memory: Invalid memory descriptor (nullptr).\n");
        return kIOReturnBadArgument;
    }
    
    VSPLog(LOG_PREFIX, "CopyMemory: CreateMapping.\n");
    
    // Access memory of TX IOMemoryDescriptor
    uint64_t mapFlags =
    kIOMemoryMapGuardedDefault |
    kIOMemoryMapCacheModeDefault |
    kIOMemoryMapReadOnly;
    ret = md->CreateMapping(mapFlags, 0, 0, 0, 0, &map);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "copy_md_memory: Failed to get memory map. code=%d\n", ret);
        return ret;
    }
    
    VSPLog(LOG_PREFIX, "CopyMemory: GetAddress + GetLength.\n");
    
    // get mapped data...
    const uint64_t mapSize = map->GetLength();
    const uint8_t* mapBuff = (uint8_t*)(map->GetAddress());
    
    VSPLog(LOG_PREFIX, "CopyMemory: dump mapped buffer\n");
    VSPLog(LOG_PREFIX, "CopyMemory MAP> mapBuff=0x%llx mapSize=%llu\n", (uint64_t) mapBuff, mapSize);
    
    // !! Debug ....
    for (uint64_t i = 0; i < mapSize && i < 16; i++) {
        VSPLog(LOG_PREFIX, "CopyMemory MAP> mapBuff[%lld]=0x%02x %c\n", i, mapBuff[i], mapBuff[i]);
    }
    
    OSSafeReleaseNULL(map);
    return kIOReturnSuccess;
}
