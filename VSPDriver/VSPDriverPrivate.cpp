// ********************************************************************
// VSPDriver - VSPDriverPrivate.cpp
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************

#include <stdio.h>
#include <os/log.h>

#include <DriverKit/OSDictionary.h>
#include <DriverKit/OSData.h>
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

#include "VSPDriverPrivate.h"
#include "VSPDriver.h"
#include "VSPLogger.h"

#define LOG_PREFIX "VSPDriverPrivate"

#define TTY_BASE_NAME "vsp"
#define SERVER_ADDRESS "127.0.0.1"
#define SERVER_PORT 9001

#define IOLockFreeNULL(l) { if (NULL != (l)) { IOLockFree(l); (l) = NULL; } }

// --------------------------------------------------------------------
//
//
VSPDriverPrivate::VSPDriverPrivate(VSPDriver* driver)
    : m_driver(driver)
    , m_provider(nullptr)
    , m_ifmd(nullptr)
    , m_rxqmd(nullptr)
    , m_txqmd(nullptr)
    , m_txAction(nullptr)
    , m_txOSData(nullptr)
    , m_rxOSData(nullptr)
    , m_txQueue(nullptr)
    , m_txDataQDSource(nullptr)
    , m_serverAddress(nullptr)
    , m_serverPort(0)
    , m_isConnected(false)
    , m_lock(nullptr)
    , m_errorState()
    , m_uartParams()
    , m_hwStatus()
    , m_hwFlowControl()
    , m_hwMCR()
    , m_fifo()
    , m_hwLatency(0)
{
    VSPLog(LOG_PREFIX, "constructor called.\n");
}

// --------------------------------------------------------------------
//
//
VSPDriverPrivate::~VSPDriverPrivate()
{
    VSPLog(LOG_PREFIX, "destructor called.\n");
    m_driver = 0L;
    m_provider = 0L;
}

// --------------------------------------------------------------------
//
//
inline void VSPDriverPrivate::CleanupResources()
{
    VSPLog(LOG_PREFIX, "CleanupResources called.\n");

    OSSafeReleaseNULL(m_txDataQDSource);
    OSSafeReleaseNULL(m_txQueue);
    OSSafeReleaseNULL(m_txAction);

    // Disconnect all queues. This deallocates
    // the INT/RX/TX buffer resources too
    m_driver->DisconnectQueues();
  
    OSSafeReleaseNULL(m_serverAddress);
    OSSafeReleaseNULL(m_txOSData);
    OSSafeReleaseNULL(m_rxOSData);

    IOLockFreeNULL(m_lock);

    // remove TX buffer ...
    if (m_fifo.tx.buffer && m_fifo.tx.size) {
        IOFree(m_fifo.tx.buffer, m_fifo.tx.size);
    }
    // remove RX buffer ...
    if (m_fifo.rx.buffer && m_fifo.rx.size) {
        IOFree(m_fifo.rx.buffer, m_fifo.rx.size);
    }
    // !! reset !!
    m_fifo = {};
}

// --------------------------------------------------------------------
//
//
IOReturn VSPDriverPrivate::SetupTTYBaseName()
{
    IOReturn ret;
    OSDictionary* properties = nullptr;
    OSString* baseName = nullptr;

    // setup custom TTY name
    if ((ret = m_driver->CopyProperties(&properties)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "Start: Unable to get driver properties. code=%d\n", ret);
        return ret;
    }
    
    baseName = OSString::withCString(TTY_BASE_NAME);
    properties->setObject(kIOTTYBaseNameKey, baseName);
    if ((ret = m_driver->SetProperties(properties)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "Start: Unable to set driver properties. code=%d\n", ret);
        //return ret;
    }
    
    OSSafeReleaseNULL(baseName);
    OSSafeReleaseNULL(properties);
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
//
//
IOReturn VSPDriverPrivate::ConnectDriverQueues()
{
    IOReturn ret;
    
    ret = m_driver->ConnectQueues(&m_ifmd, &m_rxqmd, &m_txqmd, nullptr, nullptr, 0, 0, 8, 8);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "ConnectQueues failed to allocate IF/RX/TX buffers.\n");
        return ret;
    }
    if (m_ifmd == nullptr) {
        VSPLog(LOG_PREFIX, "ConnectQueues: Invalid interrupt buffer detected.\n");
        return kIOReturnInvalid;
    }
    if (m_rxqmd == nullptr) {
        VSPLog(LOG_PREFIX, "ConnectQueues: Invalid RX buffer detected.\n");
        return kIOReturnInvalid;
    }
    if (m_txqmd == nullptr) {
        VSPLog(LOG_PREFIX, "ConnectQueues: Invalid TX buffer detected.\n");
        return kIOReturnInvalid;
    }
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
//
//
IOReturn VSPDriverPrivate::SetupFIFOBuffers()
{
    uint64_t size = 0;
    IOReturn ret;
    
    // --- allocate internal fifo TX buffer ---
    if ((ret = m_txqmd->GetLength(&size)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "SetupFIFOBuffers: Unable to get TX buffer length.\n");
        return ret;
    }
    m_fifo.tx.buffer = reinterpret_cast<char*>(IOMallocZero(size));
    if (m_fifo.tx.buffer == nullptr) {
        VSPLog(LOG_PREFIX, "SetupFIFOBuffers: Failed to allocate TX FIFO.\n");
        m_fifo.tx = {};
        return kIOReturnNoMemory;
    }
    m_fifo.tx.size = size;

    // --- allocate internal fifo RX buffer ---
    if ((ret = m_rxqmd->GetLength(&size)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "SetupFIFOBuffers: Unable to get RX buffer length.\n");
        return ret;
    }
    m_fifo.rx.buffer = reinterpret_cast<char*>(IOMallocZero(size));
    if (m_fifo.rx.buffer == nullptr) {
        VSPLog(LOG_PREFIX, "SetupFIFOBuffers: Failed to allocate RX FIFO.\n");
        m_fifo.rx = {};
        return kIOReturnNoMemory;
    }
    m_fifo.rx.size = size;

    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
//
//
IOReturn VSPDriverPrivate::Start(IOService* provider)
{
    IOReturn ret = kIOReturnSuccess;

    VSPLog(LOG_PREFIX, "Start called.\n");
    
    // remember OS provider
    m_provider = (provider ? provider : m_driver->GetProvider());
    if (!m_provider) {
        VSPLog(LOG_PREFIX, "No provider instance.\n");
        return kIOReturnInvalid;
    }
    
    m_lock = IOLockAlloc();
    if (m_lock == nullptr) {
        VSPLog(LOG_PREFIX, "Unable to allocate lock object.\n");
        return kIOReturnNoMemory;
    }

    VSPLog(LOG_PREFIX, "Connect INT/RX/TX buffers.\n");
    
    // connect dispatcher queues and get its memory descriptors
    if ((ret = ConnectDriverQueues()) != kIOReturnSuccess) {
        return ret;
    }

    // Allocate private FIFO buffers
    if ((ret = SetupFIFOBuffers()) != kIOReturnSuccess) {
        goto error_exit;
    }

    // Async notification from IODataQueueDispatchSource::DataAvailable
    ret = m_driver->CreateActionTxPacketsAvailable(m_fifo.tx.size, &m_txAction);
    if (ret != kIOReturnSuccess || m_txAction == nullptr) {
        VSPLog(LOG_PREFIX, "Start: Unable to create TX packet action. code=%d\n", ret);
        goto error_exit;
    }

    // ??? I expected that I can connect the m_txqmd, but this will not work.
    // ??? This IOUserSerial dispatch queue will not work too
    ret = m_driver->CopyDispatchQueue(kIOServiceDefaultQueueName, &m_txQueue);
    if (ret != kIOReturnSuccess || m_txQueue == nullptr) {
        VSPLog(LOG_PREFIX, "Start: m_txBuffer->CopyDispatchQueue failed. code=%d\n", ret);
        goto error_exit;
    }
    
    ret = IODataQueueDispatchSource::Create(m_fifo.tx.size, m_txQueue, &m_txDataQDSource);
    if (ret != kIOReturnSuccess || m_txDataQDSource == nullptr) {
        VSPLog(LOG_PREFIX, "Start: IODataQueueDispatchSource::Create failed. code=%d\n", ret);
        goto error_exit;
    }
    
    ret = m_txDataQDSource->SetDataAvailableHandler(m_txAction);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "Start: m_txDataQDSource->SetDataAvailableHandler failed. code=%d\n", ret);
        goto error_exit;
    }

    // Get OSData objects with FIFO buffers
    m_txOSData = OSData::withBytesNoCopy(m_fifo.tx.buffer, m_fifo.tx.size);
    if (m_txOSData == nullptr) {
        VSPLog(LOG_PREFIX, "Start: Unable to create TX packet action. code=%d\n", ret);
        goto error_exit;
    }
    
    m_rxOSData = OSData::withBytesNoCopy(m_fifo.rx.buffer, m_fifo.rx.size);
    if (m_rxOSData == nullptr) {
        VSPLog(LOG_PREFIX, "Start: Unable to create TX packet action. code=%d\n", ret);
        goto error_exit;
    }

    if ((ret = SetupTTYBaseName()) != kIOReturnSuccess) {
        goto error_exit;
    }
    
    VSPLog(LOG_PREFIX, "prepare TCP socket.\n");
    
    // Default TCP server settings
    m_serverPort = SERVER_PORT;
    m_serverAddress = OSString::withCString(SERVER_ADDRESS);
    if (!m_serverAddress) {
        ret = kIOReturnNoMemory;
        goto error_exit;
    }
    
    // TODO: Serial device
    
    // default UART parameters
    m_uartParams.baudRate = 112500;
    m_uartParams.nHalfStopBits = 1;
    m_uartParams.nDataBits = 8;
    m_uartParams.parity = 0;

    return kIOReturnSuccess;

error_exit:
    CleanupResources();
    return ret;
}

// --------------------------------------------------------------------
//
//
IOReturn VSPDriverPrivate::Stop(IOService* provider)
{
    VSPLog(LOG_PREFIX, "Stop called.\n");
    
    CleanupResources();
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
//
//
IOReturn VSPDriverPrivate::RxDataAvailable()
{
    IOReturn ret = kIOReturnSuccess;

    VSPLog(LOG_PREFIX, "RxDataAvailable called.\n");
    
    return ret;
}

// --------------------------------------------------------------------
// ??? Called by TxDataAvailable() and here we get always 0x00 in all
// ??? mapped buffer of the IOMemoryDescriptors m_txqmd, m_rxqmd and m_ifmd
//
static inline IOReturn copy_md_memory(IOMemoryDescriptor* md, char* buffer, uint64_t size)
{
    IOMemoryMap* map = nullptr;
    IOReturn ret;

    if (md == nullptr) {
        VSPLog(LOG_PREFIX, "copy_md_memory: Invalid memory descriptor (nullptr).\n");
        return kIOReturnBadArgument;
    }
    if (size == 0 || buffer == nullptr) {
        VSPLog(LOG_PREFIX, "copy_md_memory: Invalid buffer or size parameter.\n");
        return kIOReturnBadArgument;
    }

    // Access memory of TX IOMemoryDescriptor
    ret = md->CreateMapping(kIOMemoryMapReadOnly, 0, 0, 0, 0, &map);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "copy_md_memory: Failed to get memory map. code=%d\n", ret);
        return ret;
    }

    // get mapped data...
    const char* mapBuffer = reinterpret_cast<char*>(map->GetAddress());
    const uint64_t mapSize = map->GetLength();

    VSPLog(LOG_PREFIX, "copy_md_memory: mapBuffer=0x%llx mapSize=%llu\n", (uint64_t) mapBuffer, mapSize);
    VSPLog(LOG_PREFIX, "copy_md_memory: debug mapped buffer\n");
    
    // !! Debug ....
    for (uint64_t i = 0; i < mapSize && i < 16; i++) {
        VSPLog(LOG_PREFIX, "copy_md_memory> 0x%02x %c\n", mapBuffer[i], mapBuffer[i]);
    }
    
    // copy data to send into tx FIFO buffer
    memcpy(buffer, mapBuffer, (mapSize < size ? mapSize : size));

    OSSafeReleaseNULL(map);
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// Got this call event if the client app send data to /dev/serial-xxxxx
// device. But no data bytes in the IOMemoryDescriptors ??????
IOReturn VSPDriverPrivate::TxDataAvailable()
{
    IOReturn ret = kIOReturnSuccess;
    
    VSPLog(LOG_PREFIX, "TxDataAvailable called.\n");

    // Lock to ensure thread safety
    IOLockLock(m_lock);

    VSPLog(LOG_PREFIX, "TxDataAvailable: Dump m_txqmd\n");
    ret = copy_md_memory(m_txqmd, m_fifo.tx.buffer, m_fifo.tx.size);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "TxDataAvailable: copy_md_memory failed on m_txqmd\n");
        IOLockUnlock(m_lock);
        return ret;
    }

    VSPLog(LOG_PREFIX, "TxDataAvailable: Dump m_rxqmd\n");
    ret = copy_md_memory(m_rxqmd, m_fifo.rx.buffer, m_fifo.rx.size);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "TxDataAvailable: copy_md_memory failed on m_rxqmd\n");
        IOLockUnlock(m_lock);
        return ret;
    }

    VSPLog(LOG_PREFIX, "TxDataAvailable: Dump m_ifmd\n");
    ret = copy_md_memory(m_ifmd, m_fifo.tx.buffer, m_fifo.tx.size);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "TxDataAvailable: copy_md_memory failed on m_ifmd\n");
        IOLockUnlock(m_lock);
        return ret;
    }

    IOLockUnlock(m_lock);
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
//
//
IOReturn VSPDriverPrivate::TxPacketsAvailable(OSAction* action)
{
    VSPLog(LOG_PREFIX, "TxPacketsAvailable called.\n");
  
    if (action == nullptr) {
        VSPLog(LOG_PREFIX, "TxPacketsAvailable: Invalid action object (nullptr).\n");
        return kIOReturnBadArgument;
    }
    
    // Lock to ensure thread safety
    IOLockLock(m_lock);

    const uint64_t size = m_fifo.tx.size;
    const char* buffer = (char*)action->GetReference();
    if (buffer == nullptr) {
        VSPLog(LOG_PREFIX, "TxPacketsAvailable: Invalid buffer pointer (nullptr).\n");
        IOLockUnlock(m_lock);
        return kIOReturnBadArgument;
    }
    
    // !! Debug ....
    for (uint64_t i = 0; i < size && i < 16; i++) {
        VSPLog(LOG_PREFIX, "TxPacketsAvailable: TX> 0x%02x %c\n", buffer[i], buffer[i]);
    }
    
    // copy data to send into tx FIFO buffer
    memcpy(m_fifo.tx.buffer, buffer, (size < m_fifo.tx.size ? size : m_fifo.tx.size));

    // !! cleanup
    IOLockUnlock(m_lock);
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
//
//
void VSPDriverPrivate::RxFreeSpaceAvailable()
{
    VSPLog(LOG_PREFIX, "RxFreeSpaceAvailable called.\n");
}

// --------------------------------------------------------------------
//
//
void VSPDriverPrivate::TxFreeSpaceAvailable()
{
    VSPLog(LOG_PREFIX, "TxFreeSpaceAvailable called.\n");
}

// --------------------------------------------------------------------
//
//
IOReturn VSPDriverPrivate::SetModemStatus(bool cts, bool dsr, bool ri, bool dcd)
{
    VSPLog(LOG_PREFIX, "SetModemStatus called.\n");
    VSPLog(LOG_PREFIX, "CTS=%d DSR=%d RI=%d DCD=%d\n", cts, dsr, ri, dcd);
    
    m_hwStatus.cts = cts;
    m_hwStatus.dsr = dsr;
    m_hwStatus.ri  = ri;
    m_hwStatus.dcd = dcd;
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
//
//
IOReturn VSPDriverPrivate::RxError(bool overrun, bool gotBreak, bool framingError, bool parityError)
{
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
    
    m_errorState.overrun = overrun;
    m_errorState.framingError = framingError;
    m_errorState.gotBreak = gotBreak;
    m_errorState.parityError = parityError;
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
//
//
IOReturn VSPDriverPrivate::HwActivate()
{
    VSPLog(LOG_PREFIX, "HwActivate called.\n");
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
//
//
IOReturn VSPDriverPrivate::HwDeactivate()
{
    VSPLog(LOG_PREFIX, "HwDeactivate called.\n");
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
//
//
IOReturn VSPDriverPrivate::HwResetFIFO(bool tx, bool rx)
{
    VSPLog(LOG_PREFIX, "HwResetFIFO called.\n");
    VSPLog(LOG_PREFIX, "HwResetFIFO: tx=%d rx=%d\n", tx, rx);

    // TODO: FIFO implement reset
    /*if (rx) fifoResetRx(); */
    /*if (tx) fifoResetTx(); */

    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
//
//
IOReturn VSPDriverPrivate::HwSendBreak(bool sendBreak)
{
    VSPLog(LOG_PREFIX, "HwSendBreak called.\n");
    VSPLog(LOG_PREFIX, "HwSendBreak: sendBreak=%d\n", sendBreak);

    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
//
//
IOReturn VSPDriverPrivate::HwProgramUART(uint32_t baudRate, uint8_t nDataBits, uint8_t nHalfStopBits, uint8_t parity)
{
    VSPLog(LOG_PREFIX, "HwProgramUART called.\n");
    VSPLog(LOG_PREFIX, "HwProgramUART: baudRate=%d nDataBits=%d nHalfStopBits=%d parity=%d\n",
           baudRate, nDataBits, nHalfStopBits, parity);

    m_uartParams.baudRate = baudRate;
    m_uartParams.nDataBits = nDataBits;
    m_uartParams.nHalfStopBits = nHalfStopBits;
    m_uartParams.parity = parity;
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
//
//
IOReturn VSPDriverPrivate::HwProgramBaudRate(uint32_t baudRate)
{
    VSPLog(LOG_PREFIX, "HwProgramBaudRate called.\n");
    VSPLog(LOG_PREFIX, "HwProgramBaudRate: baudRate=%d\n", baudRate);

    m_uartParams.baudRate = baudRate;
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
//
//
IOReturn VSPDriverPrivate::HwProgramMCR(bool dtr, bool rts)
{
    VSPLog(LOG_PREFIX, "HwProgramMCR called.\n");
    VSPLog(LOG_PREFIX, "HwProgramMCR: dtr=%d rts=%d\n", dtr, rts);

    m_hwMCR.dtr = dtr;
    m_hwMCR.rts = rts;
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
//
//
IOReturn VSPDriverPrivate::HwGetModemStatus(bool* cts, bool* dsr, bool* ri, bool* dcd)
{
    VSPLog(LOG_PREFIX, "HwGetModemStatus called.\n");
    VSPLog(LOG_PREFIX, "HwGetModemStatus: cts=%d dsr=%d ri=%d dcd=%d\n", //
           m_hwStatus.cts, m_hwStatus.dsr, m_hwStatus.ri, m_hwStatus.dcd);

    if (cts != nullptr) {
        (*cts) = m_hwStatus.cts;
    }

    if (dsr != nullptr) {
        (*dsr) = m_hwStatus.dsr;
    }

    if (ri != nullptr) {
        (*ri) = m_hwStatus.ri;
    }

    if (dcd != nullptr) {
        (*dcd) = m_hwStatus.dcd;
    }

    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
//
//
IOReturn VSPDriverPrivate::HwProgramLatencyTimer(uint32_t latency)
{
    VSPLog(LOG_PREFIX, "HwProgramLatencyTimer called.\n");
    VSPLog(LOG_PREFIX, "HwProgramLatencyTimer: latency=%d\n", latency);
   
    m_hwLatency = latency;
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
//
//
IOReturn VSPDriverPrivate::HwProgramFlowControl(uint32_t arg, uint8_t xon, uint8_t xoff)
{
    VSPLog(LOG_PREFIX, "HwProgramFlowControl called.\n");
    VSPLog(LOG_PREFIX, "HwProgramFlowControl: arg=%02x xon=%02x xoff=%02x\n", arg, xon, xoff);
    
    m_hwFlowControl.arg = arg;
    m_hwFlowControl.xon = xon;
    m_hwFlowControl.xoff = xoff;
    
    return kIOReturnSuccess;
}
