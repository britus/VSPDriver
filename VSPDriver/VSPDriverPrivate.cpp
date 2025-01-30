//
//  VSPDriverPrivate.cpp
//  VSPDriver
//
//  Created by Björn Eschrich on 30.01.25.
//

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

VSPDriverPrivate::VSPDriverPrivate(VSPDriver* driver)
    : m_driver(driver)
    , m_provider(0L)
    , m_rxBuffer(0L)
    , m_txBuffer(0L)
    , m_serverAddress(0L)
    , m_serverPort(9001)
    , m_isConnected(false)
{
}

VSPDriverPrivate::~VSPDriverPrivate()
{
    m_driver = 0L;
    m_provider = 0L;
}

IOReturn VSPDriverPrivate::Start(IOService* provider)
{
    IOReturn ret = kIOReturnSuccess;
    
    if (!provider)
        return kIOReturnInvalid;
    
    // remember OS provider
    m_provider = provider;
    
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
    OSSafeReleaseNULL(m_serverAddress);
    OSSafeReleaseNULL(m_rxQueue);
    OSSafeReleaseNULL(m_txQueue);
    OSSafeReleaseNULL(m_rxBuffer);
    OSSafeReleaseNULL(m_txBuffer);
    return kIOReturnSuccess;
}
