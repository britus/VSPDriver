// ********************************************************************
// VSPDriver - VSPDriver.cpp
//
// Copyright © 2025 by EoF Software Labs
// SPDX-License-Identifier: MIT
// ********************************************************************

#include <stdio.h>
#include <os/log.h>

#include <DriverKit/DriverKit.h>
#include <DriverKit/IOService.h>
#include <DriverKit/IOTypes.h>
#include <DriverKit/IOUserServer.h>
#include <DriverKit/IOMemoryDescriptor.h>
#include <DriverKit/IODispatchQueue.h>
#include <DriverKit/IOUserServer.h>
#include <DriverKit/IOLib.h>

// -- SerialDriverKit
#include <SerialDriverKit/SerialDriverKit.h>
#include <SerialDriverKit/IOUserSerial.h>

#include "VSPDriverPrivate.h"
#include "VSPDriver.h"

#define DRIVER_NAME "VSP-PRIV"

VSPDriverPrivate::VSPDriverPrivate(VSPDriver* driver)
    : m_driver(driver)
    , m_provider(0L)
    , m_rxBuffer(0L)
    , m_txBuffer(0L)
    , m_serverAddress(0L)
    , m_serverPort(9001)
    , m_isConnected(false)
{
    os_log(OS_LOG_DEFAULT, "[%s] constructor called.\n", DRIVER_NAME);
}

VSPDriverPrivate::~VSPDriverPrivate()
{
    os_log(OS_LOG_DEFAULT, "[%s] deconstructor called.\n", DRIVER_NAME);
    m_driver = 0L;
    m_provider = 0L;
}

IOReturn VSPDriverPrivate::Start(IOService* provider)
{
    IOReturn ret = kIOReturnSuccess;

    os_log(OS_LOG_DEFAULT, "[%s] Start called.\n", DRIVER_NAME);

    if (!provider)
        return kIOReturnInvalid;
    
    // remember OS provider
    m_provider = provider;
    
    os_log(OS_LOG_DEFAULT, "[%s] Initialize RX/TX buffer.\n", DRIVER_NAME);
    
    // Initialize buffers
    IOBufferMemoryDescriptor::Create(kIOMemoryDirectionOut, 4096, 4, &m_txBuffer);
    if (!m_txBuffer) {
        ret = kIOReturnNoMemory;
        goto failed_exit;
    }
    
    IOBufferMemoryDescriptor::Create(kIOMemoryDirectionIn, 4096, 4, &m_rxBuffer);
    if (!m_rxBuffer) {
        ret = kIOReturnNoMemory;
        goto failed_exit;
    }

    os_log(OS_LOG_DEFAULT, "[%s] Initialize RX/TX queues.\n", DRIVER_NAME);
    
    IODispatchQueue::Create("vsp.rxQueue", 0, 0, &m_rxQueue);
    if (!m_rxQueue) {
        ret = kIOReturnNoMemory;
        goto failed_exit;
    }

    IODispatchQueue::Create("vsp.txQueue", 0, 0, &m_txQueue);
    if (!m_txQueue) {
        ret = kIOReturnNoMemory;
        goto failed_exit;
    }
   
    os_log(OS_LOG_DEFAULT, "[%s] prepare TCP socket.\n", DRIVER_NAME);
    
    // Default TCP server settings
    m_serverAddress = OSString::withCString("127.0.0.1");
    if (!m_serverAddress) {
        ret = kIOReturnNoMemory;
        goto failed_exit;
    }
    
    return kIOReturnSuccess;

failed_exit:
    OSSafeReleaseNULL(m_serverAddress);
    OSSafeReleaseNULL(m_rxQueue);
    OSSafeReleaseNULL(m_txQueue);
    OSSafeReleaseNULL(m_rxBuffer);
    OSSafeReleaseNULL(m_txBuffer);
    return ret;
}

IOReturn VSPDriverPrivate::Stop(IOService* provider)
{
    os_log(OS_LOG_DEFAULT, "[%s] Stop called.\n", DRIVER_NAME);
    OSSafeReleaseNULL(m_serverAddress);
    OSSafeReleaseNULL(m_rxQueue);
    OSSafeReleaseNULL(m_txQueue);
    OSSafeReleaseNULL(m_rxBuffer);
    OSSafeReleaseNULL(m_txBuffer);
    return kIOReturnSuccess;
}

void VSPDriverPrivate::RxDataAvailable()
{
    os_log(OS_LOG_DEFAULT, "[%s] RxDataAvailable called.\n", DRIVER_NAME);
}

void VSPDriverPrivate::TxFreeSpaceAvailable()
{
    os_log(OS_LOG_DEFAULT, "[%s] TxFreeSpaceAvailable called.\n", DRIVER_NAME);
}

IOReturn VSPDriverPrivate::SetModemStatus(bool cts, bool dsr, bool ri, bool dcd)
{
    os_log(OS_LOG_DEFAULT, "[%s] SetModemStatus called.\n", DRIVER_NAME);
    os_log(OS_LOG_DEFAULT, "[%s] CTS=%d DSR=%d RI=%d DCD=%d\n", DRIVER_NAME, cts, dsr, ri, dcd);

    return kIOReturnSuccess;
}

IOReturn VSPDriverPrivate::RxError(bool overrun, bool gotBreak, bool framingError, bool parityError)
{
    os_log(OS_LOG_DEFAULT, "[%s] RxError called.\n", DRIVER_NAME);
   
    if (overrun) {
        os_log(OS_LOG_DEFAULT, "[%s] RX overrun.\n", DRIVER_NAME);
    }

    if (gotBreak) {
        os_log(OS_LOG_DEFAULT, "[%s] RX got break.\n", DRIVER_NAME);
    }

    if (framingError) {
        os_log(OS_LOG_DEFAULT, "[%s] RX framing error.\n", DRIVER_NAME);
    }

    if (parityError) {
        os_log(OS_LOG_DEFAULT, "[%s] RX parity error.\n", DRIVER_NAME);
    }

    return kIOReturnSuccess;
}
