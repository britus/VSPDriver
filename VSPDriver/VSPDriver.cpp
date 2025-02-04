// ********************************************************************
// Driver - Driver.cpp
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

// -- SerialDriverKit
#include <SerialDriverKit/SerialDriverKit.h>
#include <SerialDriverKit/SerialPortInterface.h>
#include <SerialDriverKit/IOUserSerial.h>

// -- My
#include "VSPDriver.h"
#include "VSPLogger.h"

#define LOG_PREFIX "VSPDriver"

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

typedef struct {
    struct RX {
        char* buffer;
        uint64_t size;
    } rx;
    struct TX {
        char* buffer;
        uint64_t size;
    } tx;
} THwFIFO;

// Driver instance state resource
struct VSPDriver_IVars {
    IOService* m_provider;
    
    IOBufferMemoryDescriptor *m_ifmd;   // Interrupt related buffer
    IOMemoryDescriptor *m_txqmd;         // Transmit buffer
    IOMemoryDescriptor *m_rxqmd;         // Receive buffer
    OSAction* m_txAction;                   // Async get client TX packets action
    OSData* m_txOSData;                     // ?? for ConfigureReport
    OSData* m_rxOSData;                     // ?? for ConfigureReport

    IODispatchQueue* m_txQueue = nullptr;
    IODataQueueDispatchSource* m_txDataQDSource = nullptr;
    
    IOLock* m_lock;
    
    // Serial interface
    TErrorState m_errorState;
    TUartParameters m_uartParams;
    THwSerialStatus m_hwStatus;
    THwFlowControl m_hwFlowControl;
    THwMCR m_hwMCR;
    THwFIFO m_fifo;
    uint32_t m_hwLatency;
    
    // TCP socket connection details
    OSString *m_serverAddress;
    uint16_t m_serverPort;
    bool m_isConnected;
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
// CleanupResources_Impl() Remove all resources
//
void IMPL(VSPDriver, CleanupResources)
{
    VSPLog(LOG_PREFIX, "CleanupResources called.\n");

    OSSafeReleaseNULL(ivars->m_txDataQDSource);
    OSSafeReleaseNULL(ivars->m_txOSData);
    OSSafeReleaseNULL(ivars->m_rxOSData);
    OSSafeReleaseNULL(ivars->m_txQueue);
    OSSafeReleaseNULL(ivars->m_txAction);
    OSSafeReleaseNULL(ivars->m_serverAddress);

    // Disconnect all queues. This deallocates
    // the INT/RX/TX buffer resources too
    this->DisconnectQueues();
  
    // remove TX buffer ...
    if (ivars->m_fifo.tx.buffer && ivars->m_fifo.tx.size) {
        IOFree(ivars->m_fifo.tx.buffer, ivars->m_fifo.tx.size);
    }
    // remove RX buffer ...
    if (ivars->m_fifo.rx.buffer && ivars->m_fifo.rx.size) {
        IOFree(ivars->m_fifo.rx.buffer, ivars->m_fifo.rx.size);
    }
    // !! reset !!
    ivars->m_fifo = {};
    
    IOLockFreeNULL(ivars->m_lock);
}

// --------------------------------------------------------------------
// SetupTTYBaseName_Impl()
//
IOReturn IMPL(VSPDriver, SetupTTYBaseName)
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
IOReturn IMPL(VSPDriver, ConnectDriverQueues)
{
    IOReturn ret;
    
    ret = this->ConnectQueues(&ivars->m_ifmd,       // --
                              &ivars->m_rxqmd,      // --
                              &ivars->m_txqmd,      // --
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
// SetupFIFOBuffers_Impl()
//
IOReturn IMPL(VSPDriver, SetupFIFOBuffers)
{
    uint64_t size = 0;
    IOReturn ret;
    
    // --- allocate internal fifo TX buffer ---
    if ((ret = ivars->m_txqmd->GetLength(&size)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "SetupFIFOBuffers: Unable to get TX buffer length.\n");
        return ret;
    }
    ivars->m_fifo.tx.buffer = reinterpret_cast<char*>(IOMallocZero(size));
    if (ivars->m_fifo.tx.buffer == nullptr) {
        VSPLog(LOG_PREFIX, "SetupFIFOBuffers: Failed to allocate TX FIFO.\n");
        ivars->m_fifo.tx = {};
        return kIOReturnNoMemory;
    }
    ivars->m_fifo.tx.size = size;

    // --- allocate internal fifo RX buffer ---
    if ((ret = ivars->m_rxqmd->GetLength(&size)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "SetupFIFOBuffers: Unable to get RX buffer length.\n");
        return ret;
    }
    ivars->m_fifo.rx.buffer = reinterpret_cast<char*>(IOMallocZero(size));
    if (ivars->m_fifo.rx.buffer == nullptr) {
        VSPLog(LOG_PREFIX, "SetupFIFOBuffers: Failed to allocate RX FIFO.\n");
        ivars->m_fifo.rx = {};
        return kIOReturnNoMemory;
    }
    ivars->m_fifo.rx.size = size;

    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// ??? Called by TxDataAvailable() and here we get always 0x00 in all
// ??? mapped buffer of the IOMemoryDescriptors m_txqmd, m_rxqmd and m_ifmd
//
IOReturn IMPL(VSPDriver, CopyMemory)
{
    IOMemoryMap* map = nullptr;
    IOReturn ret;

    VSPLog(LOG_PREFIX, "CopyMemory called.\n");

    if (md == nullptr) {
        VSPLog(LOG_PREFIX, "copy_md_memory: Invalid memory descriptor (nullptr).\n");
        return kIOReturnBadArgument;
    }
    if (size == 0 || buffer == nullptr) {
        VSPLog(LOG_PREFIX, "copy_md_memory: Invalid buffer or size parameter.\n");
        return kIOReturnBadArgument;
    }
    
    VSPLog(LOG_PREFIX, "CopyMemory: reset buffer.\n");
    
    // reset targer buffer
    memset(buffer, 0, size);
    
    VSPLog(LOG_PREFIX, "CopyMemory: CreateMapping.\n");
    
    // Access memory of TX IOMemoryDescriptor
    ret = md->CreateMapping(kIOMemoryMapReadOnly, 0, 0, 0, 0, &map);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "copy_md_memory: Failed to get memory map. code=%d\n", ret);
        return ret;
    }
    
    VSPLog(LOG_PREFIX, "CopyMemory: GetAddress.\n");
    
    // get mapped data...
    const char* mapBuffer = reinterpret_cast<char*>(map->GetAddress());
    const uint64_t mapSize = map->GetLength();

    VSPLog(LOG_PREFIX, "copy_md_memory: 1 mapBuffer=0x%llx mapSize=%llu\n", (uint64_t) mapBuffer, mapSize);
    VSPLog(LOG_PREFIX, "copy_md_memory: 1 debug mapped buffer\n");
    
    // !! Debug ....
    for (uint64_t i = 0; i < mapSize && i < 16; i++) {
        VSPLog(LOG_PREFIX, "copy_md_memory_1> 0x%02x %c\n", mapBuffer[i], mapBuffer[i]);
    }
    
    char* map2Buffer  = (char*) mapBuffer;
    uint64_t map2Size = mapSize;

    VSPLog(LOG_PREFIX, "copy_md_memory: 2 mapBuffer=0x%llx mapSize=%llu\n", (uint64_t) map2Buffer, map2Size);

    ret = md->Map(kIOMemoryMapReadOnly, (uint64_t) buffer, size, 0, (uint64_t*)map2Buffer, &map2Size);
    if (ret == kIOReturnSuccess) {
        // !! Debug ....
        for (uint64_t i = 0; i < map2Size && i < 16; i++) {
            VSPLog(LOG_PREFIX, "copy_md_memory_2> 0x%02x %c\n", map2Buffer[i], map2Buffer[i]);
        }
    }
    
    VSPLog(LOG_PREFIX, "copy_md_memory: 3 mapBuffer=0x%llx mapSize=%llu\n", (uint64_t) buffer, size);

    // !! Debug ....
    for (uint64_t i = 0; i < map2Size && i < 16; i++) {
        VSPLog(LOG_PREFIX, "copy_md_memory_3> 0x%02x %c\n", buffer[i], buffer[i]);
    }

    // copy data to send into tx FIFO buffer
    memcpy(buffer, map2Buffer, (map2Size < size ? map2Size : size));

    OSSafeReleaseNULL(map);
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// Start_Impl(IOService* provider)
//
kern_return_t IMPL(VSPDriver, Start)
{
    kern_return_t ret;
  
    VSPLog(LOG_PREFIX, "start called.\n");
   
    /* check our private driver instance */
    if (!ivars) {
        VSPLog(LOG_PREFIX, "Private driver instance is NULL\n");
        return kIOReturnInvalid;
    }

    /* call apple style super method */
    ret = Start(provider, SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "Start (super): failed. code=%d\n", ret);
        return ret;
    }
  
    // the locker
    ivars->m_lock = IOLockAlloc();
    if (ivars->m_lock == nullptr) {
        VSPLog(LOG_PREFIX, "Unable to allocate lock object.\n");
        goto error_exit;
    }

    // remember OS provider
    ivars->m_provider = provider;

    // default UART parameters
    ivars->m_uartParams.baudRate = 112500;
    ivars->m_uartParams.nHalfStopBits = 1;
    ivars->m_uartParams.nDataBits = 8;
    ivars->m_uartParams.parity = 0;
    
    VSPLog(LOG_PREFIX, "Connect INT/RX/TX buffers.\n");
    
    // connect dispatcher queues and get its memory descriptors
    if ((ret = ConnectDriverQueues()) != kIOReturnSuccess) {
        goto error_exit;
    }
    
    VSPLog(LOG_PREFIX, "Create FIFO buffers.\n");

    // Allocate private FIFO buffers
    if ((ret = SetupFIFOBuffers()) != kIOReturnSuccess) {
        goto error_exit;
    }

    // Async notification from IODataQueueDispatchSource::DataAvailable
    ret = CreateActionTxPacketsAvailable(ivars->m_fifo.tx.size, &ivars->m_txAction);
    if (ret != kIOReturnSuccess || ivars->m_txAction == nullptr) {
        VSPLog(LOG_PREFIX, "Start: Unable to create TX packet action. code=%d\n", ret);
        goto error_exit;
    }

    // ??? I expected that I can connect the m_txqmd, but this will not work.
    // ??? This IOUserSerial dispatch queue will not work too
    ret = CopyDispatchQueue(kIOServiceDefaultQueueName, &ivars->m_txQueue);
    if (ret != kIOReturnSuccess || ivars->m_txQueue == nullptr) {
        VSPLog(LOG_PREFIX, "Start: m_txBuffer->CopyDispatchQueue failed. code=%d\n", ret);
        goto error_exit;
    }
    
    ret = IODataQueueDispatchSource::Create(ivars->m_fifo.tx.size, ivars->m_txQueue, &ivars->m_txDataQDSource);
    if (ret != kIOReturnSuccess || ivars->m_txDataQDSource == nullptr) {
        VSPLog(LOG_PREFIX, "Start: IODataQueueDispatchSource::Create failed. code=%d\n", ret);
        goto error_exit;
    }
    
    ret = ivars->m_txDataQDSource->SetDataAvailableHandler(ivars->m_txAction);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "Start: m_txDataQDSource->SetDataAvailableHandler failed. code=%d\n", ret);
        goto error_exit;
    }

    // Get OSData objects with FIFO buffers
    ivars->m_txOSData = OSData::withBytesNoCopy(ivars->m_fifo.tx.buffer, ivars->m_fifo.tx.size);
    if (ivars->m_txOSData == nullptr) {
        VSPLog(LOG_PREFIX, "Start: Unable to create TX packet action. code=%d\n", ret);
        goto error_exit;
    }
    
    ivars->m_rxOSData = OSData::withBytesNoCopy(ivars->m_fifo.rx.buffer, ivars->m_fifo.rx.size);
    if (ivars->m_rxOSData == nullptr) {
        VSPLog(LOG_PREFIX, "Start: Unable to create TX packet action. code=%d\n", ret);
        goto error_exit;
    }

    if ((ret = SetupTTYBaseName()) != kIOReturnSuccess) {
        goto error_exit;
    }
    
    VSPLog(LOG_PREFIX, "prepare internal stuff.\n");
    
    // Default TCP server settings
    ivars->m_serverPort = SERVER_PORT;
    ivars->m_serverAddress = OSString::withCString(SERVER_ADDRESS);
    if (ivars->m_serverAddress == nullptr) {
        ret = kIOReturnNoMemory;
        goto error_exit;
    }
   
    // Register driver instance to IOReg
    if ((ret = RegisterService()) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "RegisterService failed. code=%d\n", ret);
        goto error_exit;
    }

    VSPLog(LOG_PREFIX, "driver started successfully.\n");
    return kIOReturnSuccess;

error_exit:
    CleanupResources();
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
    
    /* remove all resources */
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
void IMPL(VSPDriver, RxDataAvailable)
{
    VSPLog(LOG_PREFIX, "RxDataAvailable called.\n");

 
    RxDataAvailable(SUPERDISPATCH);
}

// --------------------------------------------------------------------
// TxDataAvailable_Impl()
//
void IMPL(VSPDriver, TxDataAvailable)
{
    IOReturn ret = kIOReturnSuccess;
    
    VSPLog(LOG_PREFIX, "TxDataAvailable called.\n");

    // Lock to ensure thread safety
    IOLockLock(ivars->m_lock);

    // ****
    //if (ivars->m_txDataQDSource->IsDataAvailable()) {
    //    ivars->m_txDataQDSource->SendDataAvailable();
    //}
       
    VSPLog(LOG_PREFIX, "TxDataAvailable: Dump m_txqmd -------------\n");
    ret = CopyMemory(ivars->m_txqmd, ivars->m_fifo.tx.buffer, ivars->m_fifo.tx.size);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "TxDataAvailable: copy_md_memory failed on m_txqmd\n");
        goto finished;
    }

    VSPLog(LOG_PREFIX, "TxDataAvailable: Dump m_rxqmd ------------\n");
    ret = CopyMemory(ivars->m_rxqmd, ivars->m_fifo.rx.buffer, ivars->m_fifo.rx.size);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "TxDataAvailable: copy_md_memory failed on m_rxqmd\n");
        goto finished;
    }

finished:
    IOLockUnlock(ivars->m_lock);
}

// --------------------------------------------------------------------
// TxPacketsAvailable_Impl() Async callback handler
//
void IMPL(VSPDriver, TxPacketsAvailable)
{
    VSPLog(LOG_PREFIX, "TxPacketsAvailable called.\n");
  
    if (action == nullptr) {
        VSPLog(LOG_PREFIX, "TxPacketsAvailable: Invalid action object (nullptr).\n");
        return;
    }
    
    // Lock to ensure thread safety
    IOLockLock(ivars->m_lock);

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
    
    // copy data to send into tx FIFO buffer
    //memcpy(ivars->m_fifo.tx.buffer, buffer, (size < ivars->m_fifo.tx.size ? size : ivars->m_fifo.tx.size));

finished:
    IOLockUnlock(ivars->m_lock);
}

// --------------------------------------------------------------------
// RxFreeSpaceAvailable_Impl()
//
void IMPL(VSPDriver, RxFreeSpaceAvailable)
{
    VSPLog(LOG_PREFIX, "RxFreeSpaceAvailable called.\n");

    RxFreeSpaceAvailable(SUPERDISPATCH);
}

// --------------------------------------------------------------------
// TxFreeSpaceAvailable_Impl()
//
void IMPL(VSPDriver, TxFreeSpaceAvailable)
{
    VSPLog(LOG_PREFIX, "TxFreeSpaceAvailable called.\n");

    TxFreeSpaceAvailable(SUPERDISPATCH);
}

// --------------------------------------------------------------------
// SetModemStatus_Impl(bool cts, bool dsr, bool ri, bool dcd)
//
kern_return_t IMPL(VSPDriver, SetModemStatus)
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
kern_return_t IMPL(VSPDriver, RxError)
{
    kern_return_t ret = kIOReturnIOError;

    VSPLog(LOG_PREFIX, "RxError called.\n");
    
    ret = RxError(overrun, gotBreak, framingError, parityError, SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "RxError (super) failed. code=%d\n", ret);
        return ret;
    }
    
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

    return ret;
}

// --------------------------------------------------------------------
// HwActivate_Impl()
//
kern_return_t IMPL(VSPDriver, HwActivate)
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
kern_return_t IMPL(VSPDriver, HwDeactivate)
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
kern_return_t IMPL(VSPDriver, HwResetFIFO)
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
kern_return_t IMPL(VSPDriver, HwSendBreak)
{
    VSPLog(LOG_PREFIX, "HwSendBreak called.\n");
    VSPLog(LOG_PREFIX, "HwSendBreak: sendBreak=%d\n", sendBreak);

    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// HwGetModemStatus_Impl()
//
kern_return_t IMPL(VSPDriver, HwGetModemStatus)
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
kern_return_t IMPL(VSPDriver, HwProgramUART)
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
kern_return_t IMPL(VSPDriver, HwProgramBaudRate)
{
    VSPLog(LOG_PREFIX, "HwProgramBaudRate called.\n");
    VSPLog(LOG_PREFIX, "HwProgramBaudRate: baudRate=%d\n", baudRate);

    ivars->m_uartParams.baudRate = baudRate;
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// HwProgramMCR_Impl()
//
kern_return_t IMPL(VSPDriver, HwProgramMCR)
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
kern_return_t IMPL(VSPDriver, HwProgramLatencyTimer)
{
    VSPLog(LOG_PREFIX, "HwProgramLatencyTimer called.\n");
    VSPLog(LOG_PREFIX, "HwProgramLatencyTimer: latency=%d\n", latency);
   
    ivars->m_hwLatency = latency;
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// HwProgramLatencyTimer_Impl()
//
kern_return_t IMPL(VSPDriver, HwProgramFlowControl)
{
    VSPLog(LOG_PREFIX, "HwProgramFlowControl called.\n");
    VSPLog(LOG_PREFIX, "HwProgramFlowControl: arg=%02x xon=%02x xoff=%02x\n", arg, xon, xoff);
    
    ivars->m_hwFlowControl.arg = arg;
    ivars->m_hwFlowControl.xon = xon;
    ivars->m_hwFlowControl.xoff = xoff;
    
    return kIOReturnSuccess;
}
