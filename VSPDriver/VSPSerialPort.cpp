// ********************************************************************
// VSPSerialPort - Serial port implementation
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************

// -- OS
#include <os/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstring>
#include <time.h>
#include <math.h>

#include <DriverKit/OSBundle.h>
#include <DriverKit/OSString.h>
#include <DriverKit/OSNumber.h>
#include <DriverKit/OSBoolean.h>
#include <DriverKit/OSArray.h>
#include <DriverKit/OSSet.h>
#include <DriverKit/OSOrderedSet.h>
#include <DriverKit/OSPtr.h>
#include <DriverKit/OSSharedPtr.h>
#include <DriverKit/OSData.h>
#include <DriverKit/OSDictionary.h>
#include <DriverKit/OSCollection.h>
#include <DriverKit/OSCollections.h>
#include <DriverKit/OSAction.h>
#include <DriverKit/IOLib.h>
#include <DriverKit/IOTypes.h>
#include <DriverKit/IOService.h>
#include <DriverKit/IOMemoryMap.h>
#include <DriverKit/IOMemoryDescriptor.h>
#include <DriverKit/IOBufferMemoryDescriptor.h>
#include <DriverKit/IODispatchQueue.h>
#include <DriverKit/IODispatchSource.h>
#include <DriverKit/IODataQueueDispatchSource.h>

// Struktured metrics
//#include <DriverKit/IOReporter.h>
//#include <DriverKit/IOReporters.h>
//#include <DriverKit/IOReportTypes.h>
//#include <DriverKit/IOReporterDefs.h>
//#include <DriverKit/IOSimpleReporter.h>

// --
//#include <DriverKit/IOCommandPool.h>
//#include <DriverKit/IOCommand.h>

// -- SerialDriverKit
#include <SerialDriverKit/SerialDriverKit.h>
#include <SerialDriverKit/SerialPortInterface.h>
#include <SerialDriverKit/IOUserSerial.h>
using namespace driverkit::serial;

// -- My
#include "VSPSerialPort.h"
#include "VSPController.h"
#include "VSPLogger.h"
#include "VSPDriver.h"

#define LOG_PREFIX "VSPSerialPort"

#define kVSPTTYBaseName "vsp"

// Updated by SetModemStatus read by HwGetModemStatus
#define MODEM_STATUS_CTS PD_RS232_S_CTS
#define MODEM_STATUS_DSR PD_RS232_S_DSR
#define MODEM_STATUS_RI PD_RS232_S_RNG
#define MODEM_STATUS_DCD PD_RS232_S_DCD

#define MCR_STATUS_DTR PD_RS232_S_DTR
#define MCR_STATUS_RTS PD_RS232_S_RTS

#define ERROR_STATE_OVERRUN BIT(1)
#define ERROR_STATE_BREAK BIT(2)
#define ERROR_STATE_FRAME BIT(3)
#define ERROR_STATE_PARITY BIT(4)

#ifndef BIT
#define BIT(b) (1 << b)
#endif

#define IS_BIT(v, b)  ((v & b) == b)
#define SET_BIT(v, b) (v |= b)
#define RMV_BIT(v, b) (v &= ~b)
#define UPDATE_BIT(v, b, s) if (s) {SET_BIT(v,b);} else {RMV_BIT(v,b);}

// Updated by HwProgramFlowControl
typedef struct{
    uint32_t arg;
    uint8_t xon;
    uint8_t xoff;
} THwFlowControl;

// Updated by HwProgramUART and HwProgramBaudRate
typedef struct {
    uint32_t baudRate;
    uint8_t nDataBits;
    uint8_t nHalfStopBits;
    uint8_t parity;
} TUartParameters;

struct TResponseInfo {
    IOBufferMemoryDescriptor* bmd = nullptr;
    VSPSerialPort* self = nullptr;
    TResponseInfo* next = nullptr;
    uint64_t itemId = 0;
};

// Driver instance state resource
struct VSPSerialPort_IVars {
    IOService* m_provider = nullptr;                // should be the VSPDriver instance
    VSPDriver* m_parent = nullptr;                  // VSPDriver instance set by VSPDriver
    
    uint8_t m_portId = 0;                           // port id given by VSPDriver
    uint8_t m_portLinkId = 0;                       // port link id given by VSPDriver
    
    IOPropertyName m_portSuffix = {0};               // the TTY device number
    IOPropertyName m_portBaseName = {0};             // the TTY device name 'vsp'
    
    //IORecursiveLock* m_lock = nullptr;               // for resource locking
    IOLock* m_lock = nullptr;
    atomic_int m_lockCount = 0;
    
    /* OS provided memory descriptors by overridden
     * method ConnectQueues(...) */
    SerialPortInterface* m_spi;                     // OS serial port interface
    
    IOBufferMemoryDescriptor* m_txqbmd;             // Kernel TX queue memory descriptor
    IOAddressSegment m_txseg = {};                  // Kernel TX buffer segment
    
    IOBufferMemoryDescriptor* m_rxqbmd;             // Kernel RX queue memory descriptor
    IOAddressSegment m_rxseg = {};                  // Kernel RX buffer segment

    // Serial port interface
    uint32_t m_errorState = 0;
    uint32_t m_hwStatus = 0;
    uint32_t m_hwMCR = 0;
    uint32_t m_hwLatency = 25;

    TUartParameters m_uartParams = {};              // set by TTY client and VSPUserClient
    THwFlowControl m_hwFlowControl = {};            // set by TTY client and VSPUserClient

    // Data dispatch queue for sendResponse()
    atomic_int m_item_count = 0;
    atomic_int m_response_count = 0;
    TResponseInfo* m_response_head = nullptr;
    TResponseInfo* m_response_tail = nullptr;
    IODispatchQueue* m_workQueue = nullptr;
    IODispatchQueue* m_sendQueue = nullptr;
};

static inline void VSPAquireLock(VSPSerialPort_IVars* ivars)
{
    //IORecursiveLockLock(ivars->m_lock);
    IOLockLock(ivars->m_lock);
    
    //ivars->m_lockCount++;
    //VSPLog(LOG_PREFIX, "VSPAquireLock: count=%d\n", ivars->m_lockCount);
}

static inline void VSPUnlock(VSPSerialPort_IVars* ivars)
{
    //ivars->m_lockCount--;
    //VSPLog(LOG_PREFIX, "VSPUnlock...: count=%d\n", ivars->m_lockCount);

    //IORecursiveLockUnlock(ivars->m_lock);
    IOLockUnlock(ivars->m_lock);
}

// --------------------------------------------------------------------
// Allocate internal resources. Returns true if successfully initialized.
//
bool VSPSerialPort::init(void)
{
    bool result;
    
    VSPLog(LOG_PREFIX, "init called.\n");
    
    if (!(result = super::init())) {
        VSPErr(LOG_PREFIX, "super::init falsed. result=%d\n", result);
        goto error_exit;
    }
    
    // Create instance state resource
    ivars = IONewZero(VSPSerialPort_IVars, 1);
    if (!ivars) {
        VSPErr(LOG_PREFIX, "Unable to allocate driver data.\n");
        result = false;
        goto error_exit;
    }
    
    VSPLog(LOG_PREFIX, "init finished.\n");
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
// Get proptery value by specified property key using SearchProperty()
//
static inline kern_return_t getProperty(VSPSerialPort* self, const char* key, char* result, size_t size)
{
    const char* plane = "IOService:/IOUserResources/VSPDriver"; // How to get this dynamically?
    const uint32_t options = 0;//kIOServiceSearchPropertyParents;
    OSContainer* property = nullptr;
    OSString* val = nullptr;
    const char* value;
    IOReturn ret;

    if ((ret = self->SearchProperty(key, plane, options, &property)) != kIOReturnSuccess) {
        return ret;
    }
    
    if ((val = OSDynamicCast(OSString, property)) != nullptr) {
        if ((value = val->getCStringNoCopy()) != nullptr) {
            strncpy(result, value, size);
        } else {
            ret = kIOReturnInvalid;
        }
    }
    else {
        ret = kIOReturnInvalid;
    }
    
    OSSafeReleaseNULL(property);
    return ret;
}

// --------------------------------------------------------------------
// Read IOTTYBaseName and IOTTYSuffix from service properties
//
static inline kern_return_t readTTYProperties(VSPSerialPort* self)
{
    IOReturn ret = kIOReturnSuccess;
    
    //VSPLog(LOG_PREFIX, "readTTYProperties called.\n");
    if (strlen(self->ivars->m_portBaseName) == 0) {
        ret = getProperty(self, "IOTTYBaseName", self->ivars->m_portBaseName, sizeof(IOPropertyName)-1);
        if (ret != kIOReturnSuccess) {
            VSPErr(LOG_PREFIX, "readTTYProperties: getProperty(IOTTYBaseName) failed. code=%x", ret);
            goto finish;
        }
    }
    if (strlen(self->ivars->m_portSuffix) == 0) {
        ret = getProperty(self, "IOTTYSuffix", self->ivars->m_portSuffix, sizeof(IOPropertyName)-1);
        if (ret != kIOReturnSuccess) {
            VSPErr(LOG_PREFIX, "readTTYProperties: getProperty(IOTTYSuffix) failed. code=%x", ret);
            goto finish;
        }
    }

finish:
    return ret;
}

// --------------------------------------------------------------------
// Start_Impl(IOService* provider)
//
kern_return_t IMPL(VSPSerialPort, Start)
{
    // The size of the queue in bytes.
    kern_return_t ret;

    VSPLog(LOG_PREFIX, "Start: called.\n");
    
    // sane check our driver instance vars
    if (!ivars) {
        VSPErr(LOG_PREFIX, "Start: Private driver instance is NULL\n");
        return kIOReturnInvalid;
    }
    
    // remember OS provider before Start. Start calls
    // SetProperties implicit and we update the TTY
    // properties there. Therfore we need the provider
    // which is our VSPDriver instance.
    ivars->m_provider = provider;
    
    // call super method (apple style)
    ret = Start(provider, SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "Start(super): failed. code=%x\n", ret);
        return ret;
    }
    
    // the resource locker
    //ivars->m_lock = IORecursiveLockAlloc();
    ivars->m_lock = IOLockAlloc();
    if (ivars->m_lock == nullptr) {
        VSPErr(LOG_PREFIX, "Start: Unable to allocate lock object.\n");
        goto error_exit;
    }
    
    // default UART parameters
    ivars->m_uartParams.baudRate = 115200;
    ivars->m_uartParams.nHalfStopBits = 2;
    ivars->m_uartParams.nDataBits = 8;
    ivars->m_uartParams.parity = PD_RS232_PARITY_DEFAULT;

    // --
    
    VSPLog(LOG_PREFIX, "Start: register service.\n");
    
    // Register driver instance to IOReg
    if ((ret = RegisterService()) != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "Start: RegisterService failed. code=%x\n", ret);
        goto error_exit;
    }
    
    if (readTTYProperties(this) != kIOReturnSuccess) {
        goto error_exit;
    }

    VSPLog(LOG_PREFIX, "Start: Port started successfully.\n");
    return kIOReturnSuccess;
    
error_exit:
    cleanupResources();
    return ret;
}

// --------------------------------------------------------------------
// Stop_Impl(IOService* provider)
//
kern_return_t IMPL(VSPSerialPort, Stop)
{
    kern_return_t ret;
    
    VSPLog(LOG_PREFIX, "Stop called.\n");
    
    // Remove all IVars resources
    cleanupResources();
    
    /* Shutdown instane */
    if ((ret= Stop(provider, SUPERDISPATCH)) != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "super::Stop failed. code=%x\n", ret);
    } else {
        VSPLog(LOG_PREFIX, "Port successfully removed.\n");
    }
    
    return ret;
}

// --------------------------------------------------------------------
// SetProperties(OSDictionary* properties)
// Modify instance properties. Change IOTTYBaseName to "vsp"
// and set our unique port number to IOTTYSuffix
kern_return_t IMPL(VSPSerialPort, SetProperties)
{
    IOReturn ret;
    OSString* key;
    OSString* val;
    VSPDriver* parent;

    VSPLog(LOG_PREFIX, "SetProperties called.\n");
    
    if (properties == nullptr) {
        return kIOReturnBadArgument;
    }
    
    // Tage next port id
    if ((parent = OSDynamicCast(VSPDriver, ivars->m_provider))) {
        parent->getNextPortId(&ivars->m_portId);
    }
    else {
        VSPLog(LOG_PREFIX, "Start: Provider is not VSPDriver !!!!!!!!\n");
        return kIOReturnInvalid;
    }
    
    strncpy(ivars->m_portBaseName, "vsp", sizeof(IOPropertyName)-1);
    snprintf(ivars->m_portSuffix, sizeof(IOPropertyName), "%d", ivars->m_portId);
    
    VSPLog(LOG_PREFIX, "SetProperties: property count=%d\n", properties->getCount());
    
    // Update TTY base name with our 'vsp'
    key = OSString::withCString(kIOTTYBaseNameKey);
    val = OSString::withCString(ivars->m_portBaseName);
    if (properties->setObject(key, val)) {
        VSPLog(LOG_PREFIX, "SetProperties: TTY base name updated with 'vsp'.\n");
    }
    OSSafeReleaseNULL(val);
    OSSafeReleaseNULL(key);
    
    // Update suffix with our port number
    key = OSString::withCString(kIOTTYSuffixKey);
    val = OSString::withCString(ivars->m_portSuffix);
    if (properties->setObject(key, val)) {
        VSPLog(LOG_PREFIX, "SetProperties: property IOTTYSuffix updated with portId=%d.\n", ivars->m_portId);
    }
    OSSafeReleaseNULL(val);
    OSSafeReleaseNULL(key);

    // Save properties
    ret = SetProperties(properties, SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "SetProperties returned=%x, try UserSetProperties\n", ret);
        ret = UserSetProperties(properties);
        if (ret != kIOReturnSuccess) {
            VSPErr(LOG_PREFIX, "SetProperties: UserSetProperties() failed. code=%x", ret);
        }
    }
    
    VSPLog(LOG_PREFIX, "SetProperties complete with code=%x\n", ret);
    return ret;
}

// --------------------------------------------------------------------
// Remove all resources in IVars
//
void VSPSerialPort::cleanupResources()
{
    VSPLog(LOG_PREFIX, "cleanupResources called.\n");
    
    // Unlink VSP
    ivars->m_parent = nullptr;
    ivars->m_portId = 0;
    ivars->m_portLinkId = 0;
    
    //IORecursiveLockFreeZero(ivars->m_lock);
    IOLockFreeZero(ivars->m_lock);
}

bool VSPSerialPort::isConnected()
{
    return (ivars->m_txqbmd && ivars->m_rxqbmd && ivars->m_spi);
}

// ====================================================================
// ** ----------------[ Connection live cycle ]--------------------- **
// ====================================================================

// --------------------------------------------------------------------
// ConnectQueues_Impl( ... )
// First call
kern_return_t IMPL(VSPSerialPort, ConnectQueues)
{
    IOAddressSegment ifseg = {};
    size_t txcapacity, rxcapacity;
    IOReturn ret;
    char queueId[255];
    
    VSPLog(LOG_PREFIX, "ConnectQueues called\n");
    
    //-- Sane check --//
    if (!in_txqlogsz || !in_rxqlogsz) {
        VSPErr(LOG_PREFIX, "ConnectQueues: Invalid in_rxqlogsz or in_txqlogsz detected.\n");
        return kIOReturnBadArgument;
    }
    
    // Lock to ensure thread safety
    VSPAquireLock(ivars);
    
    if (ivars->m_txqbmd || ivars->m_rxqbmd) {
        VSPErr(LOG_PREFIX, "ConnectQueues: Called TWICE! Failing with error\n");
        ret = kIOReturnAborted;
        goto error_exit;
    }
    
    // Convert the base-2 logarithmic size of the buffer or the in_txqmd parameter.
    txcapacity = (size_t) ::pow(2, in_txqlogsz);
    ret = IOBufferMemoryDescriptor::Create(kIOMemoryDirectionIn, txcapacity, 0, &ivars->m_txqbmd);
    if (ret != kIOReturnSuccess || !ivars->m_txqbmd) {
        VSPErr(LOG_PREFIX, "ConnectQueues: Unable to create TX memory descriptor. code=%x\n", ret);
        goto error_exit;
    }
    
    // Convert the base-2 logarithmic size of the buffer for the in_rxqmd parameter.
    rxcapacity = (size_t) ::pow(2, in_rxqlogsz);
    ret = IOBufferMemoryDescriptor::Create(kIOMemoryDirectionOut, rxcapacity, 0, &ivars->m_rxqbmd);
    if (ret != kIOReturnSuccess || !ivars->m_rxqbmd) {
        VSPErr(LOG_PREFIX, "ConnectQueues: Unable to create RX memory descriptor. code=%x\n", ret);
        goto error_exit;
    }

    // make sure the parameters are zero
    in_rxqoffset = 0;
    in_txqoffset = 0;
    
    // Call super to get SerialPortInterface and set our RX/TX memory descriptors
    ret = ConnectQueues(ifmd, rxqmd, txqmd,
                        ivars->m_rxqbmd,
                        ivars->m_txqbmd,
                        in_rxqoffset,
                        in_txqoffset,
                        in_rxqlogsz,
                        in_txqlogsz, SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "super::ConnectQueues failed. code=%x\n", ret);
        goto error_exit;
    }
    
    //-- Sane check --//
    if (!ifmd || !(*ifmd) || !txqmd || !(*txqmd) || !rxqmd || !(*rxqmd)) {
        VSPErr(LOG_PREFIX, "ConnectQueues: Invalid memory descriptors detected. (NULL)\n");
        ret = kIOReturnBadArgument;
        goto error_exit;
    }
    if ((*txqmd) != ivars->m_txqbmd) {
        VSPErr(LOG_PREFIX, "ConnectQueues: Invalid 'txqmd' memory descriptor detected.\n");
        ret = kIOReturnInvalid;
        goto error_exit;
    }
    if ((*rxqmd) != ivars->m_rxqbmd) {
        VSPErr(LOG_PREFIX, "ConnectQueues: Invalid 'rxqmd' memory descriptor detected.\n");
        ret = kIOReturnInvalid;
        goto error_exit;
    }

    // Get the address segment of the TX memory descriptor
    ret = ivars->m_txqbmd->GetAddressRange(&ivars->m_txseg);
    if (ret != kIOReturnSuccess || !ivars->m_txseg.address || !ivars->m_txseg.length) {
        VSPErr(LOG_PREFIX, "ConnectQueues: Unable to get TX-MD segment. code=%x\n", ret);
        goto error_exit;
    }
    
    // Get the address segment of the RX memory descriptor
    ret = ivars->m_rxqbmd->GetAddressRange(&ivars->m_rxseg);
    if (ret != kIOReturnSuccess || !ivars->m_rxseg.address || !ivars->m_rxseg.length) {
        VSPErr(LOG_PREFIX, "ConnectQueues: Unable to get RX-MD segment. code=%x\n", ret);
        goto error_exit;
    }
    
    // Get the address segment of the SerialPortInterface (shared space with kernel)
    if ((ret = (*ifmd)->GetAddressRange(&ifseg)) != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "ConnectQueues: IF GetAddressRange failed. code=%x\n", ret);
        goto error_exit;
    }

    // Initialize the SPI indexes
    ivars->m_spi = reinterpret_cast<SerialPortInterface*>(ifseg.address);
    if (!ivars->m_spi) {
        VSPErr(LOG_PREFIX, "ConnectQueues: Invalid 'ifseg' address detected.\n");
        ret = kIOReturnInvalid;
        goto error_exit;
    }

    ivars->m_spi->txCI = 0;
    ivars->m_spi->txPI = 0;
    ivars->m_spi->rxCI = 0;
    ivars->m_spi->rxPI = 0;
    
    // --
    // Create internal data dispatching for sendResponse() action
    // --
    snprintf(queueId, 251, "VSPSerialPort.queue.c%s%s", //
             ivars->m_portBaseName, ivars->m_portSuffix);
    
    ret = IODispatchQueue::Create(queueId, 0, 0, &ivars->m_workQueue);
    if (ret != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "Start: IODispatchQueue::Create failed with error: 0x%08x.", ret);
        goto error_exit;
    }
    
    snprintf(queueId, 251, "VSPSerialPort.queue.s%s%s", //
             ivars->m_portBaseName, ivars->m_portSuffix);
    
    ret = IODispatchQueue::Create(queueId, 0, 0, &ivars->m_sendQueue);
    if (ret != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "Start: IODispatchQueue::Create failed with error: 0x%08x.", ret);
        goto error_exit;
    }
  
    ivars->m_item_count = 0;
    ivars->m_response_head = nullptr;
    ivars->m_response_tail = nullptr;
    ivars->m_response_count = 0;

    VSPUnlock(ivars);

    // Modem is ready
    setDataCarrierDetect(true);
    setClearToSend(true);
    
    return kIOReturnSuccess;
    
error_exit:
    ivars->m_txseg = {};
    ivars->m_rxseg = {};
    ivars->m_spi = nullptr;
    OSSafeReleaseNULL(ivars->m_sendQueue);
    OSSafeReleaseNULL(ivars->m_workQueue);
    OSSafeReleaseNULL(ivars->m_txqbmd);
    OSSafeReleaseNULL(ivars->m_rxqbmd);
    VSPUnlock(ivars);
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

    // reset SPI pointer from OS
    ivars->m_spi = nullptr;
    
    // reset our TX/RX segments
    ivars->m_txseg = {};
    ivars->m_rxseg = {};

    // reset response list
    ivars->m_item_count = 0;
    ivars->m_response_head = nullptr;
    ivars->m_response_tail = nullptr;
    ivars->m_response_count = 0;

    // Remove worker queues
    OSSafeReleaseNULL(ivars->m_sendQueue);
    OSSafeReleaseNULL(ivars->m_workQueue);

    // Remove our memory descriptors
    OSSafeReleaseNULL(ivars->m_txqbmd);
    OSSafeReleaseNULL(ivars->m_rxqbmd);

    // unlock before HwDeactivate, prevent race condition
    VSPUnlock(ivars);
 
    ret = DisconnectQueues(SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "super::DisconnectQueues: failed. code=%x\n", ret);
        return ret;
    }
    
    return ret;
}

// --------------------------------------------------------------------
// Return driver trace flags
//
uint64_t VSPSerialPort::traceFlags()
{
    if (!ivars->m_parent) {
        return 0;
    }
    return ivars->m_parent->traceFlags();
}

// --------------------------------------------------------------------
// Return driver trace flags
//
uint64_t VSPSerialPort::checkFlags()
{
    if (!ivars->m_parent) {
        return 0;
    }
    return ivars->m_parent->checkFlags();
}

// --------------------------------------------------------------------
// Return TTY base name with device number.
//
kern_return_t VSPSerialPort::getDeviceName(char* result, const uint32_t size)
{
    IOReturn ret;
    
    if (!result || size < (strlen(ivars->m_portBaseName) + strlen(ivars->m_portSuffix))) {
        return kIOReturnBadArgument;
    }
    
    // Get current TTY properties.
    if ((ret = readTTYProperties(this)) != kIOReturnSuccess) {
        return ret;
    }

    strncpy(result, ivars->m_portBaseName, size);
    strncat(result, ivars->m_portSuffix, size);
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// Enable/Disable modem status clear to send
//
void VSPSerialPort::setClearToSend(bool cts)
{
    bool modified = false;
    
    VSPAquireLock(ivars);
    if (IS_BIT(ivars->m_hwStatus, MODEM_STATUS_CTS) != cts) {
        UPDATE_BIT(ivars->m_hwStatus, MODEM_STATUS_CTS, cts);
        modified = true;
    }
    VSPUnlock(ivars);
    
    if (modified) {
        reportModemStatus();
    }
}

// --------------------------------------------------------------------
// Enable/Disable modem status data set ready
//
void VSPSerialPort::setDataSetReady(bool dsr)
{
    bool modified = false;
    
    VSPAquireLock(ivars);
    if (IS_BIT(ivars->m_hwStatus, MODEM_STATUS_DSR) != dsr) {
        UPDATE_BIT(ivars->m_hwStatus, MODEM_STATUS_DSR, dsr);
        modified = true;
    }
    VSPUnlock(ivars);
    
    if (modified) {
        reportModemStatus();
    }
}

// --------------------------------------------------------------------
// Enable/Disable modem status ring indicator
//
void VSPSerialPort::setRingIndicator(bool ri)
{
    bool modified = false;
    
    VSPAquireLock(ivars);
    if (IS_BIT(ivars->m_hwStatus, MODEM_STATUS_RI) != ri) {
        UPDATE_BIT(ivars->m_hwStatus, MODEM_STATUS_RI, ri);
        modified = true;
    }
    VSPUnlock(ivars);
    
    if (modified) {
        reportModemStatus();
    }
}

// --------------------------------------------------------------------
// Enable/Disable modem status data carrier detect
//
void VSPSerialPort::setDataCarrierDetect(bool dcd)
{
    bool modified = false;
    
    VSPAquireLock(ivars);
    if (IS_BIT(ivars->m_hwStatus, MODEM_STATUS_DCD) != dcd) {
        UPDATE_BIT(ivars->m_hwStatus, MODEM_STATUS_DCD, dcd);
        modified = true;
    }
    VSPUnlock(ivars);
    
    if (modified) {
        reportModemStatus();
    }
}

// --------------------------------------------------------------------
// Report current modem status to the system
//
kern_return_t VSPSerialPort::reportModemStatus()
{
    VSPAquireLock(ivars);
    IOReturn ret = SetModemStatus(
            IS_BIT(ivars->m_hwStatus, MODEM_STATUS_CTS),
            IS_BIT(ivars->m_hwStatus, MODEM_STATUS_DSR),
            IS_BIT(ivars->m_hwStatus, MODEM_STATUS_RI),
            IS_BIT(ivars->m_hwStatus, MODEM_STATUS_DCD), SUPERDISPATCH);
    VSPUnlock(ivars);

    if (ret != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "super::SetModemStatus failed. code=%x\n", ret);
    }
    
    return ret;
}

// --------------------------------------------------------------------
// SetModemStatus_Impl(bool cts, bool dsr, bool ri, bool dcd)
// Called during serial port setup or communication
kern_return_t IMPL(VSPSerialPort, SetModemStatus)
{
    IOReturn ret;
    
    if (traceFlags() & TRACE_PORT_IO) {
        VSPLog(LOG_PREFIX, "SetModemStatus called [in] CTS=%d DSR=%d RI=%d DCD=%d\n",
               cts, dsr, ri, dcd);
    }
    
    VSPAquireLock(ivars);
    UPDATE_BIT(ivars->m_hwStatus, MODEM_STATUS_CTS, cts);
    UPDATE_BIT(ivars->m_hwStatus, MODEM_STATUS_DSR, dsr);
    UPDATE_BIT(ivars->m_hwStatus, MODEM_STATUS_RI, ri);
    UPDATE_BIT(ivars->m_hwStatus, MODEM_STATUS_DCD, dcd);
    VSPUnlock(ivars);
    
    ret = SetModemStatus(cts, dsr, ri, dcd, SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "super::SetModemStatus failed. code=%x\n", ret);
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
    
    if (traceFlags() & TRACE_PORT_IO) {
        VSPLog(LOG_PREFIX, "RxError called.\n");
        if (overrun) {
            VSPLog(LOG_PREFIX, "Overrun detected.\n");
        }
        if (gotBreak) {
            VSPLog(LOG_PREFIX, "Got break.\n");
        }
        if (framingError) {
            VSPLog(LOG_PREFIX, "Framing error detected.\n");
        }
        if (parityError) {
            VSPLog(LOG_PREFIX, "Parity error detected.\n");
        }
    }
    
    VSPAquireLock(ivars);
    UPDATE_BIT(ivars->m_errorState, ERROR_STATE_OVERRUN, overrun);
    UPDATE_BIT(ivars->m_errorState, ERROR_STATE_FRAME, framingError);
    UPDATE_BIT(ivars->m_errorState, ERROR_STATE_BREAK, gotBreak);
    UPDATE_BIT(ivars->m_errorState, ERROR_STATE_PARITY, parityError);
    VSPUnlock(ivars);
    
    ret = RxError(overrun, gotBreak, framingError, parityError, SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "super::RxError: failed. code=%x\n", ret);
        return ret;
    }
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// HwActivate_Impl()
// Called after ConnectQueues() or other reasons
kern_return_t IMPL(VSPSerialPort, HwActivate)
{
    setDataCarrierDetect(true);
    setDataSetReady(false);
    setClearToSend(false);
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// HwDeactivate_Impl()
// Called before DisconnectQueues() or other reasons
kern_return_t IMPL(VSPSerialPort, HwDeactivate)
{
    setDataCarrierDetect(false);
    setDataSetReady(false);
    setClearToSend(false);
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// HwResetFIFO_Impl()
// Called by client to TxFreeSpaceAvailable or RxFreeSpaceAvailable
// or other reasons.
kern_return_t IMPL(VSPSerialPort, HwResetFIFO)
{
    if (traceFlags() & TRACE_PORT_IO) {
        VSPLog(LOG_PREFIX, "HwResetFIFO called -> tx=%d rx=%d\n",
               tx, rx);
    }

    if (!ivars->m_spi) {
        return kIOReturnNotOpen;
    }
    
    bool txNotify = false;
    bool rxNotify = false;

    if (tx) {
        VSPAquireLock(ivars);
        if (ivars->m_spi->txCI != 0) {
            ivars->m_spi->txCI = 0;
            txNotify = true;
        }
        if (ivars->m_spi->txPI != 0) {
            ivars->m_spi->txPI = 0;
            txNotify = true;
        }
        VSPUnlock(ivars);
    }
       
    if (rx) {
        VSPAquireLock(ivars);
        if (ivars->m_spi->rxCI != 0) {
            ivars->m_spi->rxCI = 0;
            rxNotify = true;
        }
        if (ivars->m_spi->rxPI != 0) {
            ivars->m_spi->rxPI = 0;
            rxNotify = true;
        }
        VSPUnlock(ivars);
    }
    

    if (txNotify) {
        TxFreeSpaceAvailable();
    }
    
    if (rxNotify) {
        RxFreeSpaceAvailable();
    }

    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// HwSendBreak_Impl()
// Called during client communication life cycle
kern_return_t IMPL(VSPSerialPort, HwSendBreak)
{
    if (traceFlags() & TRACE_PORT_IO) {
        VSPLog(LOG_PREFIX, "HwSendBreak called -> sendBreak=%d\n", sendBreak);
    }
    
    VSPAquireLock(ivars);
    UPDATE_BIT(ivars->m_errorState, ERROR_STATE_BREAK, sendBreak);
    VSPUnlock(ivars);
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// HwGetModemStatus_Impl()
// Called during client communication life cycle
kern_return_t IMPL(VSPSerialPort, HwGetModemStatus)
{
    VSPAquireLock(ivars);
    if (traceFlags() & TRACE_PORT_IO) {
        VSPLog(LOG_PREFIX, "HwGetModemStatus called [out] CTS=%d DSR=%d RI=%d DCD=%d\n", //
               IS_BIT(ivars->m_hwStatus, MODEM_STATUS_CTS),
               IS_BIT(ivars->m_hwStatus, MODEM_STATUS_DSR),
               IS_BIT(ivars->m_hwStatus, MODEM_STATUS_RI),
               IS_BIT(ivars->m_hwStatus, MODEM_STATUS_DCD));
    }
    if (cts != nullptr) {
        (*cts) = IS_BIT(ivars->m_hwStatus, MODEM_STATUS_CTS);
    }
    if (dsr != nullptr) {
        (*dsr) = IS_BIT(ivars->m_hwStatus, MODEM_STATUS_DSR);
    }
    if (ri != nullptr) {
        (*ri) = IS_BIT(ivars->m_hwStatus, MODEM_STATUS_RI);
    }
    if (dcd != nullptr) {
        (*dcd) = IS_BIT(ivars->m_hwStatus, MODEM_STATUS_DCD);
    }
    VSPUnlock(ivars);

    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// HwProgramUART_Impl()
// Called during serial port setup or other reason
kern_return_t IMPL(VSPSerialPort, HwProgramUART)
{
    if (traceFlags() & TRACE_PORT_IO) {
        VSPLog(LOG_PREFIX, "HwProgramUART called -> baudRate=%d "
               "nDataBits=%d nHalfStopBits=%d parity=%d\n",
               baudRate, nDataBits, nHalfStopBits, parity);
    }
    
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
    if (traceFlags() & TRACE_PORT_IO) {
        VSPLog(LOG_PREFIX, "HwProgramBaudRate called -> baudRate=%d\n", baudRate);
    }
    
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
    if (traceFlags() & TRACE_PORT_IO) {
        VSPLog(LOG_PREFIX, "HwProgramMCR called -> DTR=%d RTS=%d\n",
               dtr, rts);
    }
    
    VSPAquireLock(ivars);
    UPDATE_BIT(ivars->m_hwMCR, MCR_STATUS_DTR, dtr);
    UPDATE_BIT(ivars->m_hwMCR, MCR_STATUS_RTS, rts);
    VSPUnlock(ivars);
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// HwProgramLatencyTimer_Impl()
// Called during serial port setup or other reason
kern_return_t IMPL(VSPSerialPort, HwProgramLatencyTimer)
{
    if (traceFlags() & TRACE_PORT_IO) {
        VSPLog(LOG_PREFIX, "HwProgramLatencyTimer called -> latency=%d\n",
               latency);
    }
    
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
    if (traceFlags() & TRACE_PORT_IO) {
        VSPLog(LOG_PREFIX, "HwProgramFlowControl called -> arg=%02x xon=%02x xoff=%02x\n",
               arg, xon, xoff);
    }
    
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
void VSPSerialPort::setPortItem(VSPDriver* parent, void* data)
{
    if (!parent || !data) {
        return;
    }
    
    // Link parent to this instance
    ivars->m_parent = parent;

    // Save instance for VSP controller
    TVSPPortItem* item = reinterpret_cast<TVSPPortItem*>(data);
    item->id = ivars->m_portId;
    item->port = this;
    item->flags = (parent->checkFlags() | parent->traceFlags());
}

// --------------------------------------------------------------------
// Remove link to VSPDriver instance
//
void VSPSerialPort::unlinkParent()
{
    if (traceFlags() & TRACE_PORT_IO) {
        VSPLog(LOG_PREFIX, "unlinkParent called.\n");
    }
    ivars->m_parent = nullptr;
}

// --------------------------------------------------------------------
// Set the serial port identifier
//
void VSPSerialPort::setPortIdentifier(uint8_t id)
{
    if (traceFlags() & TRACE_PORT_IO) {
        VSPLog(LOG_PREFIX, "setPortIdentifier id=%d.\n", id);
    }
    
    ivars->m_portId = id;
}

// --------------------------------------------------------------------
// Get the serial port identifier
//
uint8_t VSPSerialPort::getPortIdentifier()
{
    return ivars->m_portId;
}

// --------------------------------------------------------------------
// Set the serial port link identifier
//
void VSPSerialPort::setPortLinkIdentifier(uint8_t id)
{
    if (traceFlags() & TRACE_PORT_IO) {
        VSPLog(LOG_PREFIX, "setPortLinkIdentifier id=%d at port=%d\n",
               id, ivars->m_portId);
    }
    
    VSPAquireLock(ivars);
    ivars->m_portLinkId = id;
    VSPUnlock(ivars);
}

// --------------------------------------------------------------------
// Get the serial port link identifier
//
uint8_t VSPSerialPort::getPortLinkIdentifier()
{
    return ivars->m_portLinkId;
}

// --------------------------------------------------------------------
//
//
static inline void dbg_dump_buffer(const char* prefix, const uint8_t* data, uint64_t size)
{
#ifdef DEBUG
    char hex[4] = {0};
    char dump[1024] = {0};

    for(uint64_t i=0; i < 30 && i < size; i++) {
        snprintf(hex, 4, "%02x ", data[i]);
        strlcat(dump, hex, 1023);
    }

    VSPLog(LOG_PREFIX, "Dump-%{public}s buffer=0x%llx len=%llu\n",
           prefix, (uint64_t)data, (unsigned long long)size);
    VSPLog(LOG_PREFIX, "%{public}s\n", (const char*) dump);
#endif
}

// --------------------------------------------------------------------
//
//
kern_return_t VSPSerialPort::enqueueResponse(void* context)
{
    if (!context) {
        VSPErr(LOG_PREFIX, "enqueueResponse: Invalid parameter.\n");
        return kIOReturnBadArgument;
    }
    
    TResponseInfo* ctx = reinterpret_cast<TResponseInfo*>(context);

    VSPAquireLock(ivars);
    {
        // Sort the list by itemId (lowest itemId first)
        TResponseInfo** current = &ivars->m_response_head;
        TResponseInfo* previous = nullptr;
        
        // Find the correct position to insert
        while (*current && (*current)->itemId < ctx->itemId) {
            previous = *current;
            current = &((*current)->next);
        }
        
        // Insert the new node
        ctx->next = *current;
        
        if (previous) {
            previous->next = ctx;
        } else {
            ivars->m_response_head = ctx;
        }
        
        // Update tail if needed
        TResponseInfo* temp = ctx;
        while (temp->next) {
            temp = temp->next;
        }
        ivars->m_response_tail = temp;
        
        // increment item counter
        ivars->m_response_count++;
    }
    VSPUnlock(ivars);
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
//
//
kern_return_t VSPSerialPort::dequeueResponse(void** context)
{
    if (!context) {
        VSPErr(LOG_PREFIX, "dequeueResponse: Invalid parameter.\n");
        return kIOReturnBadArgument;
    }

    TResponseInfo* ctx;
    
    VSPAquireLock(ivars);
    {
        // get first entry
        if (!(ctx = ivars->m_response_head)) {
            VSPUnlock(ivars);
            return kIOReturnNotFound;
        }
        
        // next of ctx become first
        ivars->m_response_head = ctx->next;
        
        // end of list?
        if (!ivars->m_response_head) {
            ivars->m_response_tail = nullptr;
        }
        
        // removed from list, no ref.
        ctx->next = nullptr;
        
        if (ivars->m_response_count > 0) {
            ivars->m_response_count--;
        }
    }
    VSPUnlock(ivars);

    (*context) = ctx;
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
//
//
void VSPSerialPort::triggerDispatch()
{
    void* context = nullptr;

    IOReturn ret = dequeueResponse(&context);
    if (ret != kIOReturnSuccess && ret != kIOReturnNotFound) {
        VSPErr(LOG_PREFIX, "triggerDispatch: dequeueResponse failed with ret=0x%08x\n", ret);
        return;
    }
    else if (ret == kIOReturnNotFound || !context) {
        return;
    }

    ivars->m_sendQueue->DispatchAsync(^{
        this->dispatchResponse(context);
    });
}

// --------------------------------------------------------------------
//
//
void VSPSerialPort::dispatchResponse(void* context)
{
    if (!context) {
        VSPErr(LOG_PREFIX, "dispatchResponse: Invalid parameter detected.\n");
        return;
    }

    TResponseInfo* ctx = reinterpret_cast<TResponseInfo*>(context);
    IOAddressSegment seg = {};
    IOReturn ret;

    if (traceFlags() & TRACE_PORT_RX) {
        VSPLog(LOG_PREFIX, "dispatchResponse: itemId=%llu rxPI=%u rxCI=%u\n",
               ctx->itemId, ivars->m_spi->rxPI, ivars->m_spi->rxCI);
    }

    ret = ctx->bmd->GetAddressRange(&seg);
    if (ret != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "dispatchResponse: Failed to get memory descriptor segment.\n");
        OSSafeReleaseNULL(ctx->bmd);
        IOSafeDeleteNULL(ctx, TResponseInfo, 1);
        return;
    }
    if (!seg.length || !seg.address) {
        VSPErr(LOG_PREFIX, "dispatchResponse: Invalid response BMD segment detected.\n");
        OSSafeReleaseNULL(ctx->bmd);
        IOSafeDeleteNULL(ctx, TResponseInfo, 1);
        return;
    }
        
    void* buffer = reinterpret_cast<void*>(seg.address);
   
    // call response routing if available
    if (ivars->m_portLinkId) {
        ret = sendToPortLink(ctx, buffer, seg.length);
    } else {
        ret = sendResponse(ctx, buffer, seg.length);
    }
    if (ret == kIOReturnNoSpace) {
        enqueueResponse(ctx);
        return;
    }
    
    VSPAquireLock(ivars);
    OSSafeReleaseNULL(ctx->bmd);
    IOSafeDeleteNULL(ctx, TResponseInfo, 1);
    if (ivars->m_response_count == 0) {
        ivars->m_item_count = 0;
    }
    VSPUnlock(ivars);

finish:
    triggerDispatch();
}

// --------------------------------------------------------------------
//
//
bool VSPSerialPort::scheduleResponse(uint8_t* buffer, uint64_t size)
{
    TResponseInfo* ctx;
    IOBufferMemoryDescriptor* bmd;
    IOAddressSegment seg = {};
    IOReturn ret;

    // Prepare response memory descriptor for dispatching
    ret = IOBufferMemoryDescriptor::Create(kIOMemoryDirectionInOut, size, 0, &bmd);
    if (ret != kIOReturnSuccess || !bmd) {
        VSPErr(LOG_PREFIX, "scheduleResponse: Failed to allocate RX action memory descriptor. ret=0x%08x\n", ret);
        return false;
    }
    
    // Get mapped address and buffer length
    ret = bmd->GetAddressRange(&seg);
    if (ret != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "scheduleResponse: Failed to get RX action memory segment. ret=0x%08x\n", ret);
        OSSafeReleaseNULL(bmd);
        return false;
    }
    
    // Sanity-Check
    if (seg.address == 0 || seg.length < size) {
        VSPErr(LOG_PREFIX, "scheduleResponse: Invalid RX action memory segment.\n");
        OSSafeReleaseNULL(bmd);
        return false;
    }
    
    ctx = IONewZero(TResponseInfo, 1);
    if (!ctx) {
        VSPErr(LOG_PREFIX, "scheduleResponse: Cannot allocate response command.\n");
        OSSafeReleaseNULL(bmd);
        return false;
    }

    // fill response buffer with TX data
    memcpy(reinterpret_cast<uint8_t*>(seg.address), buffer, size);
    
    if ((traceFlags() & TRACE_PORT_TX) && (traceFlags() & TRACE_PORT_IO)) {
        dbg_dump_buffer("TX", reinterpret_cast<uint8_t*>(seg.address), size);
    }
 
    // setup response action command
    VSPAquireLock(ivars);
    ivars->m_item_count++;
    ctx->itemId = ivars->m_item_count;
    ctx->next = nullptr;
    ctx->bmd = bmd;
    ctx->self = this;
    VSPUnlock(ivars);

    if (traceFlags() & TRACE_PORT_RX) {
        VSPLog(LOG_PREFIX, "scheduleResponse: itemId=%llu rxPI=%u rxCI=%u\n",
               ctx->itemId, ivars->m_spi->rxPI, ivars->m_spi->rxCI);
    }

    enqueueResponse(ctx);
    return true;
}

// --------------------------------------------------------------------
// TxDataAvailable_Impl()
// TX data ready to read from m_txqbmd segment
void IMPL(VSPSerialPort, TxDataAvailable)
{
    if (!ivars->m_spi || !ivars->m_txqbmd) {
        VSPErr(LOG_PREFIX, "TxDataAvailable: SPI or TX memory descriptor is null");
        return;
    }

    // Show me indexes be fore manipulate
    if (traceFlags() & TRACE_PORT_TX) {
        VSPLog(LOG_PREFIX, "TxDataAvailable: [IOSPI-TX 1] txPI=%u txCI=%u capacity=%llu",
               ivars->m_spi->txPI, ivars->m_spi->txCI, ivars->m_txseg.length);
    }
    
    uint8_t* base = reinterpret_cast<uint8_t*>(ivars->m_txseg.address);
    if (!base) {
        VSPErr(LOG_PREFIX, "TxDataAvailable: Cannot get TX buffer base pointer.\n");
        return;
    }
    
    const uint64_t capacity = ivars->m_txseg.length;
    if (capacity == 0) {
        VSPErr(LOG_PREFIX, "TxDataAvailable: TX buffer capacity is zero.\n");
        return;
    }

    /* end reached??, notify OS for more data, raise CTS */
    if (ivars->m_spi->txPI == ivars->m_spi->txCI) {
        setClearToSend(true);
        TxFreeSpaceAvailable();
        return;
    }
    
    // We are working
    setClearToSend(false);

    bool processed = false;
    while (ivars->m_spi->txPI != ivars->m_spi->txCI) {

        const uint64_t txPI = ivars->m_spi->txPI;
        const uint64_t txCI = ivars->m_spi->txCI;
        uint64_t bytesRead = 0;
        uint64_t toRead = 0;
        bool isWrapping = false;
        
        if (txPI >= txCI && txPI < capacity) { // Linear region
            if ((toRead = txPI - txCI) > 0) {
                if (!scheduleResponse(base + txCI, toRead)) {
                    goto finish;
                }
                bytesRead += toRead;
                processed = true;
            }
        }
        else { // Wrapped region
            if ((toRead = capacity - txCI) > 0) {
                if (!scheduleResponse(base + txCI, toRead)) {
                    goto finish;
                }
                bytesRead += toRead;
            }
            // get bytes left (read from beginning of the TX ring buffer)
            // txPI incremented over capacity with number of bytes left
            if ((toRead = txPI - capacity) > 0) {
                if (!scheduleResponse(base, toRead)) {
                    goto finish;
                }
                bytesRead += toRead;
            }
            processed = (bytesRead > 0);
            isWrapping = true;
        }

        // Lock to ensure thread safety
        VSPAquireLock(ivars);
        // Advance consumer index.
        ivars->m_spi->txCI = static_cast<uint32_t>((txCI + bytesRead) % capacity);
        // Set the producer index to the consumer index of the TX ring buffer if
        // wrap-around. The kernel does not replace the producer index with the
        // consumer index if the consumer index is less as the call before. On
        // overflow, the driver assumes that the producer (kernel) writes the bytes
        // exceeding the capacity of the TX ring buffer to the beginning. This behavior
        // cannot be explained otherwise if the difference between the producer index
        // and the capacity of the TX ring buffer exactly corresponds to the number
        // of bytes exceeding the capacity of the TX ring buffer.
        // (see above where "txPI >= txCI && txPI < capacity" checked.)
        if (isWrapping) {
            ivars->m_spi->txPI = ivars->m_spi->txCI;
        }
        VSPUnlock(ivars);

        // Show me indexes of final state
        if (traceFlags() & TRACE_PORT_TX) {
            VSPLog(LOG_PREFIX, "TxDataAvailable: [IOSPI-TX 2] txPI=%u txCI=%u read=%llu",
                   ivars->m_spi->txPI, ivars->m_spi->txCI, bytesRead);
        }

        // Trigger response queue
        triggerDispatch();
        
    } // while

finish:
    if (processed) {
        setClearToSend(true);
        TxFreeSpaceAvailable();
    }
}

// --------------------------------------------------------------------
// RxFreeSpaceAvailable_Impl()
// Notification to this instance that RX buffer space of the OS
// is available for your device's (this driver) data
void IMPL(VSPSerialPort, RxFreeSpaceAvailable)
{
    VSPAquireLock(ivars);
    if (ivars->m_spi->rxPI >= ivars->m_rxseg.length) {
        if (traceFlags() & TRACE_PORT_RX) {
            VSPLog(LOG_PREFIX, "RxFreeSpaceAvailable: Reset rxPI +++++++\n");
        }
        ivars->m_spi->rxPI = static_cast<uint32_t>(ivars->m_spi->rxPI % ivars->m_rxseg.length);
    }
    if (ivars->m_spi->rxCI >= ivars->m_rxseg.length) {
        if (traceFlags() & TRACE_PORT_RX) {
            VSPLog(LOG_PREFIX, "RxFreeSpaceAvailable: Reset rxCI +++++++\n");
        }
        ivars->m_spi->rxCI = static_cast<uint32_t>(ivars->m_spi->rxCI % ivars->m_rxseg.length);
    }
    VSPUnlock(ivars);

    triggerDispatch();
}

// --------------------------------------------------------------------
// Called by response dispatch queue (callback dispatchResponse)
// in case of 'port is linked' state
//
kern_return_t VSPSerialPort::sendToPortLink(void* context, const void* buffer, const uint64_t size)
{
    IOReturn ret;
    TVSPLinkItem item = {};
    VSPSerialPort* port = nullptr;
    
    uint8_t myLinkId = ivars->m_portLinkId;
    uint8_t myPortId = ivars->m_portId;

    if (traceFlags() & TRACE_PORT_RX) {
        VSPLog(LOG_PREFIX, "sendToPortLink: Using linkId=%d myPortId=%d\n", myLinkId, myPortId);
    }
    
    ret = ivars->m_parent->getPortLinkById(myLinkId, &item, sizeof(TVSPLinkItem));
    if (ret != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "sendToPortLink: Parent getPortLinkById failed. code=%x\n", ret);
        return ret;
    }

    if (traceFlags() & TRACE_PORT_RX) {
        VSPLog(LOG_PREFIX, "sendToPortLink: Have port link. targetId=%d\n", item.target.id);
    }
    
    if (item.source.id != myPortId) {
        port = item.source.port;
    }
    else if (item.target.id != myPortId) {
        port = item.target.port;
    }
    else {
        VSPErr(LOG_PREFIX, "sendToPortLink: Double port IDs detectd! myLinkId=%d myPortId=%d\n",
               myLinkId, myPortId);
        return kIOReturnInvalid;
    }
    if (!port) {
        VSPErr(LOG_PREFIX, "sendToPortLink: Linked port NULL pointer! myLinkId=%d myPortId=%d\n",
               myLinkId, ivars->m_portId);
        return kIOReturnInvalid;
    }
   
    if (traceFlags() & TRACE_PORT_RX) {
        VSPLog(LOG_PREFIX, "sendToPortLink: Send on target portId=%d\n",
               port->getPortIdentifier());
    }

    if (!port->isConnected()) {
        VSPErr(LOG_PREFIX, "sendToPortLink: Port %d is disconnected, skip\n",
               port->getPortIdentifier());
        return kIOReturnNotOpen;
    }
    
    if ((ret = port->sendResponse(context, buffer, size)) != kIOReturnSuccess) {
        return ret;
    }

    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// Called by response dispatch queue (callback dispatchResponse)
//
kern_return_t VSPSerialPort::sendResponse(void* context, const void* buffer, const uint64_t sizeIn)
{
    // Sanety check
    if (!buffer || !sizeIn) {
        VSPErr(LOG_PREFIX, "sendResponse: Buffer NULL or size zero.\n");
        return kIOReturnBadArgument;
    }
    
    // Skip if not connected
    if (!isConnected()) {
        VSPErr(LOG_PREFIX, "sendResponse: This port %d is closed.\n", ivars->m_portId);
        return kIOReturnNotOpen;
    }

    TResponseInfo* ctx = reinterpret_cast<TResponseInfo*>(context);
    const uint8_t* src = reinterpret_cast<const uint8_t*>(buffer);
    uint8_t* base = nullptr;
    uint64_t bytesToWrite = sizeIn;
    uint64_t written = 0;
    uint64_t capacity;

    // --- Get RX buffer info ---
    base = reinterpret_cast<uint8_t*>(ivars->m_rxseg.address);
    if (!base) {
        VSPErr(LOG_PREFIX, "sendResponse: Cannot get RX buffer base pointer.\n");
        return kIOReturnVMError;
    }

    capacity = ivars->m_rxseg.length;
    if (capacity == 0) {
        VSPErr(LOG_PREFIX, "sendResponse: RX buffer capacity is zero.\n");
        return kIOReturnVMError;
    }
    
    VSPAquireLock(ivars);
    // Check buffer overflow condition
    if (ivars->m_spi->rxPI >= ivars->m_rxseg.length) {
        if (ivars->m_spi->rxPI == ivars->m_spi->rxCI) {
            if (traceFlags() & TRACE_PORT_RX) {
                VSPLog(LOG_PREFIX, "sendResponse: Reset rxPI and rxCI\n");
            }
            ivars->m_spi->rxPI = 0;
            ivars->m_spi->rxCI = 0;
        }
        else {
            if (traceFlags() & TRACE_PORT_RX) {
                VSPLog(LOG_PREFIX, "sendResponse: Buffer full! enqueue again.\n");
            }
            VSPUnlock(ivars);
            return kIOReturnNoSpace;
        }
    }
    const uint64_t rxPI = ivars->m_spi->rxPI;
    const uint64_t rxCI = ivars->m_spi->rxCI;
    VSPUnlock(ivars);
    
    if (traceFlags() & TRACE_PORT_RX) {
        VSPLog(LOG_PREFIX, "sendResponse: [IOSPI-RX-1] itemId=%llu rxPI=%llu rxCI=%llu capacity=%llu sizeIn=%llu\n",
               ctx->itemId, rxPI, rxCI, capacity, bytesToWrite);
    }

    // +++ Copy data to ring buffer +++
    
    // Linear copy ...
    if ((rxPI + bytesToWrite) <= capacity) {
        if (traceFlags() & TRACE_PORT_RX) {
            VSPLog(LOG_PREFIX, "sendResponse: [IOSPI-RX-L] itemId=%llu rxPI=%llu rxCI=%llu toWrite=%llu\n",
                   ctx->itemId, rxPI, rxCI, bytesToWrite);
        }
        
        memcpy(base + rxPI, src, bytesToWrite);
        
        if ((traceFlags() & TRACE_PORT_RX) && (traceFlags() & TRACE_PORT_IO)) {
            dbg_dump_buffer("RX", base + rxPI, bytesToWrite);
        }
        
        written = bytesToWrite;
    }
    // Wrap-around copy
    else {
        uint64_t firstChunk  = capacity - rxPI;
        uint64_t secondChunk = bytesToWrite - firstChunk;

        if (traceFlags() & TRACE_PORT_RX) {
            VSPLog(LOG_PREFIX, "sendResponse: [IOSPI-RX-W] itemId=%llu rxPI=%llu rxCI=%llu FIRST=%llu SECOND=%llu\n",
                   ctx->itemId, rxPI, rxCI, firstChunk, secondChunk);
        }
        
        if (firstChunk > 0) {
            // Copy first chunk to end of buffer
            memcpy(base + rxPI, src, firstChunk);
            
            if ((traceFlags() & TRACE_PORT_RX) && (traceFlags() & TRACE_PORT_IO)) {
                dbg_dump_buffer("R1", base + rxPI, firstChunk);
            }
        }

        // Copy second chunk to beginning of ring buffer
        if (secondChunk > 0) {
            // Copy second chunk to top of ring buffer
            memcpy(base, src + firstChunk, secondChunk);
 
            if ((traceFlags() & TRACE_PORT_RX) && (traceFlags() & TRACE_PORT_IO)) {
                dbg_dump_buffer("R2", base, secondChunk);
            }
        }
        
        written = firstChunk + secondChunk;
    }

    // --- Update producer index ---
    VSPAquireLock(ivars);
    ivars->m_spi->rxPI = static_cast<uint32_t>((rxPI + written)/* % capacity*/);
    VSPUnlock(ivars);

    if (traceFlags() & TRACE_PORT_RX) {
        VSPLog(LOG_PREFIX, "sendResponse: [IOSPI-RX-3] itemId=%llu rxPI=%u rxCI=%u bytesWritten=%llu\n",
               ctx->itemId, ivars->m_spi->rxPI, ivars->m_spi->rxCI, written);
    }

    // --- Notify data availability ---
    if (written > 0) {
        // raise DSR signal
        setDataSetReady(true);
        // notify OS ready to read from ring buffer
        RxDataAvailable();
    }

    return kIOReturnSuccess;
}
