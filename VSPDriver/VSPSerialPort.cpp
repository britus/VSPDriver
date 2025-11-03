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
#include <DriverKit/IOInterruptDispatchSource.h>
#include <DriverKit/IOTimerDispatchSource.h>

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

#ifndef IOLockFreeNULL
#define IOLockFreeNULL(l) { if (NULL != (l)) { IOLockFree(l); (l) = NULL; } }
#endif

#ifndef BIT
#define BIT(b) (1 << b)
#endif

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

// Driver instance state resource
struct VSPSerialPort_IVars {
    IOService* m_provider = nullptr;                // should be the VSPDriver instance
    VSPDriver* m_parent = nullptr;                  // VSPDriver instance set by VSPDriver
    
    uint8_t m_portId = 0;                           // port id given by VSPDriver
    uint8_t m_portLinkId = 0;                       // port link id given by VSPDriver
    
    IOPropertyName m_portSuffix = {0};               // the TTY device number
    IOPropertyName m_portBaseName = {0};             // the TTY device name 'vsp'
    
    IOLock* m_lock = nullptr;                       // for resource locking
    volatile atomic_int m_lockLevel = 0;
    
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

    // TX data dispatch queue for sendResponse()
    IODispatchQueue* m_responseQueue = NULL;
};

struct TResponseActionInfo {
    IOBufferMemoryDescriptor* bmd = NULL;
    bool isWrapping = false;
    uint64_t txBufferSize = 0;
    uint64_t txSecondChunk = 0;
    VSPSerialPort* self = NULL;
};


static inline void VSPAquireLock(VSPSerialPort_IVars* ivars)
{
    if (!ivars->m_lockLevel) {
        if (IOLockTryLock(ivars->m_lock)) {
            ++ivars->m_lockLevel;
        }
    }
}

static inline void VSPUnlock(VSPSerialPort_IVars* ivars)
{
    if (ivars->m_lockLevel) {
        --ivars->m_lockLevel;
        IOLockUnlock(ivars->m_lock);
    }
}

// --------------------------------------------------------------------
// Called by TxDataAvailable() or sendToPortLink() to dispatch RX data
//
template <typename T>
static inline constexpr const T& MIN(const T& a, const T& b)
{
    return (a < b) ? a : b;
}

// --------------------------------------------------------------------
// Allocate internal resources. Returns true if successfully
// initialized.
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
    char queueId[255];

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
    // Create internal data dispatching for sendResponse() action
    // --
    snprintf(queueId, 251, "VSPSerialPort.queue.%s%s", //
             ivars->m_portBaseName, ivars->m_portSuffix);
 
    ret = IODispatchQueue::Create(queueId, 0, 0, &ivars->m_responseQueue);
    if (ret != kIOReturnSuccess)
    {
        VSPErr(LOG_PREFIX, "Start: IODispatchQueue::Create failed with error: 0x%08x.", ret);
        goto error_exit;
    }
    
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

    VSPLog(LOG_PREFIX, "cleanupResources removing ivars->m_rcvqds\n");
#if 0
    OSSafeReleaseNULL(ivars->m_dqdsResponse);
    OSSafeReleaseNULL(ivars->m_respAction);
#endif
    OSSafeReleaseNULL(ivars->m_responseQueue);
    IOLockFreeNULL(ivars->m_lock);
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

    // Modem is ready
    setDataCarrierDetect(true);
    setClearToSend(true);
    
    VSPUnlock(ivars);
    return kIOReturnSuccess;
    
error_exit:
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
// RxDataAvailable_Impl()
// Notify OS response RX data ready for client
void IMPL(VSPSerialPort, RxDataAvailable)
{
    if (traceFlags() & TRACE_PORT_IO) {
        VSPLog(LOG_PREFIX, "RxDataAvailable called.\n");
    }
    
    RxDataAvailable(SUPERDISPATCH);
}

// --------------------------------------------------------------------
// RxFreeSpaceAvailable_Impl()
// Notification to this instance that RX buffer space of the OS
// is available for your device's (this driver) data
void IMPL(VSPSerialPort, RxFreeSpaceAvailable)
{
    //if (traceFlags() & TRACE_PORT_IO) {
        VSPLog(LOG_PREFIX, "RxFreeSpaceAvailable called.\n");
    //}
    
    //if (!IS_BIT(ivars->m_hwStatus, MODEM_STATUS_CTS)) {
    //    setClearToSend(true);
    //}

    //RxFreeSpaceAvailable(SUPERDISPATCH);
}

// --------------------------------------------------------------------
// TxFreeSpaceAvailable_Impl()
// Notify OS ready for more client data
void IMPL(VSPSerialPort, TxFreeSpaceAvailable)
{
    if (traceFlags() & TRACE_PORT_IO) {
        VSPLog(LOG_PREFIX, "TxFreeSpaceAvailable called.\n");
    }
    
    if (!IS_BIT(ivars->m_hwStatus, MODEM_STATUS_CTS)) {
        setClearToSend(true);
    }

    TxFreeSpaceAvailable(SUPERDISPATCH);
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
    if (IS_BIT(ivars->m_hwStatus, MODEM_STATUS_CTS) != cts) {
        VSPAquireLock(ivars);
        UPDATE_BIT(ivars->m_hwStatus, MODEM_STATUS_CTS, cts);
        VSPUnlock(ivars);
        reportModemStatus();
    }
}

// --------------------------------------------------------------------
// Enable/Disable modem status data set ready
//
void VSPSerialPort::setDataSetReady(bool dsr)
{
    if (IS_BIT(ivars->m_hwStatus, MODEM_STATUS_DSR) != dsr) {
        VSPAquireLock(ivars);
        UPDATE_BIT(ivars->m_hwStatus, MODEM_STATUS_DSR, dsr);
        VSPUnlock(ivars);
        reportModemStatus();
    }
}

// --------------------------------------------------------------------
// Enable/Disable modem status ring indicator
//
void VSPSerialPort::setRingIndicator(bool ri)
{
    if (IS_BIT(ivars->m_hwStatus, MODEM_STATUS_RI) != ri) {
        VSPAquireLock(ivars);
        UPDATE_BIT(ivars->m_hwStatus, MODEM_STATUS_RI, ri);
        VSPUnlock(ivars);
        reportModemStatus();
    }
}

// --------------------------------------------------------------------
// Enable/Disable modem status data carrier detect
//
void VSPSerialPort::setDataCarrierDetect(bool dcd)
{
    if (IS_BIT(ivars->m_hwStatus, MODEM_STATUS_DCD) != dcd) {
        VSPAquireLock(ivars);
        UPDATE_BIT(ivars->m_hwStatus, MODEM_STATUS_DCD, dcd);
        VSPUnlock(ivars);
        reportModemStatus();
    }
}

// --------------------------------------------------------------------
// Report current modem status to the system
//
kern_return_t VSPSerialPort::reportModemStatus()
{
    IOReturn ret = SetModemStatus(
            IS_BIT(ivars->m_hwStatus, MODEM_STATUS_CTS),
            IS_BIT(ivars->m_hwStatus, MODEM_STATUS_DSR),
            IS_BIT(ivars->m_hwStatus, MODEM_STATUS_RI),
            IS_BIT(ivars->m_hwStatus, MODEM_STATUS_DCD), SUPERDISPATCH);
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
    
    VSPAquireLock(ivars);
    bool notify = false;
    if (ivars->m_spi) {
        if (tx) {
            if (ivars->m_spi->txCI != 0) {
                ivars->m_spi->txCI = 0;
                notify = true;
            }
            if (ivars->m_spi->txPI != 0) {
                ivars->m_spi->txPI = 0;
                notify = true;
            }
            if (notify) {
                TxFreeSpaceAvailable();
                notify = false;
            }
        }
       
        if (rx) {
            if (ivars->m_spi->rxCI != 0) {
                ivars->m_spi->rxCI = 0;
                notify = true;
            }
            if (ivars->m_spi->rxPI != 0) {
                ivars->m_spi->rxPI = 0;
                notify = true;
            }
            if (notify) {
                RxFreeSpaceAvailable();
            }
        }
    }
    VSPUnlock(ivars);
    
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
    ivars->m_portLinkId = id;
}

// --------------------------------------------------------------------
// Get the serial port link identifier
//
uint8_t VSPSerialPort::getPortLinkIdentifier()
{
    return ivars->m_portLinkId;
}

static void dispatchResponse(void* context)
{
    TResponseActionInfo* ctx = reinterpret_cast<TResponseActionInfo*>(context);
    IOAddressSegment seg = {};
    
    IOReturn ret = ctx->bmd->GetAddressRange(&seg);
    if (ret != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "dispatchResponse: Failed to get memory descriptor segment.\n");
        return;
    }
    if (seg.length != ctx->txBufferSize) {
        VSPErr(LOG_PREFIX, "dispatchResponse: Invalid buffer size in BMD segment detected.\n");
        VSPErr(LOG_PREFIX, "dispatchResponse: seg.len=%llu ctx.txSize=%llu ctx.bLeft=%llu ctx.isWrapped=%d",
               seg.length, ctx->txBufferSize, ctx->txSecondChunk, ctx->isWrapping);
        return;
    }

    void* buffer = reinterpret_cast<void*>(seg.address);
   
    // call response routing if available
    if (ctx->self->ivars->m_portLinkId) {
        ctx->self->sendToPortLink(ctx, buffer, seg.length);
    } else {
        ctx->self->sendResponse(ctx, buffer, seg.length);
    }
 
    // cleanup callers resources
    OSSafeReleaseNULL(ctx->bmd);
    IOSafeDeleteNULL(ctx, TResponseActionInfo, 1);
}

#ifdef DEBUG
static inline void dbg_dump_buffer(const uint8_t* data, uint64_t size)
{
    char hex[4] = {0};
    char dump[1024] = {0};

    for(uint64_t i=0; i < 30 && i < size; i++) {
        snprintf(hex, 4, "%02x ", data[i]);
        strlcat(dump, hex, 1023);
    }

    VSPLog(LOG_PREFIX, "Dump buffer=0x%llx len=%llu\n",
           (uint64_t)data, (unsigned long long)size);
    VSPLog(LOG_PREFIX, "%{public}s\n", (const char*) dump);
}
#endif

// --------------------------------------------------------------------
// TxDataAvailable_Impl()
// TX data ready to read from m_txqbmd segment
void IMPL(VSPSerialPort, TxDataAvailable)
{
    if (traceFlags() & TRACE_PORT_TX) {
        VSPLog(LOG_PREFIX, "--------------------------------------------------\n");
        VSPLog(LOG_PREFIX, "TxDataAvailable called.\n");
    }
    
    // Lock to ensure thread safety
    VSPAquireLock(ivars);

    if (!ivars->m_spi || !ivars->m_txqbmd) {
        VSPErr(LOG_PREFIX, "TxDataAvailable: SPI or TX memory descriptor is null");
        VSPUnlock(ivars);
        return;
    }

    // Show me indexes be fore manipulate
    if (traceFlags() & TRACE_PORT_TX) {
        VSPLog(LOG_PREFIX, "TxDataAvailable: [IOSPI-TX 1] txPI=%u txCI=%u capacity=%llu",
               ivars->m_spi->txPI, ivars->m_spi->txCI, ivars->m_txseg.length);
    }
    
    // We are working
    setClearToSend(false);
  
    const uint64_t capacity = ivars->m_txseg.length;
    uint8_t* base = reinterpret_cast<uint8_t*>(ivars->m_txseg.address);
    bool dataProcessed = false;
    IOReturn ret;

    /* end reached??, notfy OS for more data, raise CTS */
    if (ivars->m_spi->txPI == ivars->m_spi->txCI) {
#if 0
        if ((capacity - ivars->m_spi->txCI) < capacity) {
            VSPErr(LOG_PREFIX, "TxDataAvailable: Wait for data! txPI=%u == txCI=%u capacity=%llu",
                   ivars->m_spi->txPI, ivars->m_spi->txCI, ivars->m_txseg.length);
        }
#endif
        // notify OS space available
        TxFreeSpaceAvailable();
        // raise CTS
        setClearToSend(true);
        VSPUnlock(ivars);
        return;
    }

    while (ivars->m_spi->txPI != ivars->m_spi->txCI) {
        
        const uint64_t txPI = ivars->m_spi->txPI;
        const uint64_t txCI = ivars->m_spi->txCI;
        IOBufferMemoryDescriptor* bmdResponse = NULL;
        IOAddressSegment segResponse = {};
        uint8_t* kTxBuff = NULL;
        uint8_t* respbuff = NULL;
        uint64_t bytesRead = 0;
        uint64_t firstChunk = 0;
        uint64_t secondChunk = 0;
        bool isWrapping = false;

        // Linear region
        if (txPI >= txCI && txPI < capacity) {
            firstChunk = txPI - txCI;
        }
        // Wrapped region
        else {
            firstChunk = capacity - txCI;
            // get bytes left (read from beginning of the TX ring buffer)
            // txPI incremented over capacity with number of bytes left
            if (txPI > capacity) {
                secondChunk = txPI - capacity;
            }
            isWrapping = true;
        }
        
        // Strong sane check
        if (!isWrapping && ((firstChunk + secondChunk) > capacity || txCI >= capacity || txPI >= capacity)) {
            VSPErr(LOG_PREFIX,
                   "TxDataAvailable: Index corruption detected - "
                   "OS BUG? txPI=%llu txCI=%llu size=%llu cap=%llu bleft=%llu w=%d",
                   txPI, txCI, firstChunk, capacity, secondChunk, isWrapping);
            // reset consumer and producer
            ivars->m_spi->txCI = 0;
            ivars->m_spi->txPI = 0;
            TxFreeSpaceAvailable();
            goto finish;
        }

        // Prepare response memory descriptor for dispatching
        ret = IOBufferMemoryDescriptor::Create(kIOMemoryDirectionInOut, firstChunk + secondChunk, 0, &bmdResponse);
        if (ret != kIOReturnSuccess || !bmdResponse) {
            VSPErr(LOG_PREFIX, "TxDataAvailable: Failed to allocate RX action memory descriptor.\n");
            goto not_routed;
        }
        // Get mapped address and buffer length
        ret = bmdResponse->GetAddressRange(&segResponse);
        if (ret != kIOReturnSuccess) {
            VSPErr(LOG_PREFIX, "TxDataAvailable: Failed to get RX action memory segment.\n");
            OSSafeReleaseNULL(bmdResponse);
            goto not_routed;
        }
        // Sanity-Check
        if (segResponse.address == 0 || segResponse.length < (firstChunk + secondChunk)) {
            VSPErr(LOG_PREFIX, "TxDataAvailable: Invalid RX action memory segment.\n");
            OSSafeReleaseNULL(bmdResponse);
            goto not_routed;
        }
        
        respbuff = reinterpret_cast<uint8_t*>(segResponse.address);
        
        // get buffer address by offset
        kTxBuff = base + txCI;

        // Show me indexes
        if (traceFlags() & TRACE_PORT_TX) {
            VSPLog(LOG_PREFIX, "TxDataAvailable: [IOSPI-TX 2] txPI=%llu txCI=%llu size=%llu ktxbuff=%p",
                   txPI, txCI, firstChunk, kTxBuff);
        }

        // Linear copy
        if (!isWrapping) {
            // Show me indexes
            if (traceFlags() & TRACE_PORT_TX) {
                VSPLog(LOG_PREFIX, "TxDataAvailable: [IOSPI-TX L] txPI=%llu txCI=%llu first=%llu second=%llu",
                       txPI, txCI, firstChunk, secondChunk);
            }
            
            // Copy available TX data block to RX action memory descriptor
            memcpy(respbuff, kTxBuff, firstChunk);
            bytesRead += firstChunk;

            if ((traceFlags() & TRACE_PORT_TX) /*&& (traceFlags() & TRACE_PORT_IO)*/) {
                dbg_dump_buffer(respbuff, firstChunk);
            }
        }
        // wrap-around copy
        else {
            // Show me indexes
            if (traceFlags() & TRACE_PORT_TX) {
                VSPLog(LOG_PREFIX, "TxDataAvailable: [IOSPI-TX W] txPI=%llu txCI=%llu first=%llu second=%llu",
                       txPI, txCI, firstChunk, secondChunk);
            }
            
            // Copy first chunk from the TX ring buffer
            memcpy(respbuff, kTxBuff, firstChunk);
            bytesRead += firstChunk;

            if ((traceFlags() & TRACE_PORT_TX) /*&& (traceFlags() & TRACE_PORT_IO)*/) {
                dbg_dump_buffer(respbuff, firstChunk);
            }

            // Copy the second chunk from begining of the TX ring buffer
            memcpy(respbuff + firstChunk, base, secondChunk);
            bytesRead += secondChunk;

            if ((traceFlags() & TRACE_PORT_TX) /*&& (traceFlags() & TRACE_PORT_IO)*/) {
                dbg_dump_buffer(respbuff + firstChunk, secondChunk);
            }
        }

        // ---------------------------------------------------------------
        // Dispatch data for response
        TResponseActionInfo* ractx;
        ractx = IONewZero(TResponseActionInfo, 1);
        ractx->isWrapping = isWrapping;
        ractx->txBufferSize = bytesRead;
        ractx->txSecondChunk = secondChunk;
        ractx->bmd = bmdResponse;
        ractx->self = this;
        ivars->m_responseQueue->DispatchAsync_f(ractx, dispatchResponse);
        // Dispatch END
        // --------------------------------------------------------------
        
        // data processing complete
        dataProcessed = true;

not_routed:
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

        // Show me indexes of final state
        if (traceFlags() & TRACE_PORT_TX) {
            VSPLog(LOG_PREFIX, "TxDataAvailable: [IOSPI-TX 3] txPI=%u txCI=%u read=%llu",
                   ivars->m_spi->txPI, ivars->m_spi->txCI, bytesRead);
        }
    } // while

finish:
    if (dataProcessed) {
        setClearToSend(true);
        TxFreeSpaceAvailable();
    }

    if (traceFlags() & TRACE_PORT_TX) {
        VSPLog(LOG_PREFIX, "TxDataAvailable complete.\n");
    }
    
    VSPUnlock(ivars);
}

// --------------------------------------------------------------------
// Called by TxDataAvailable in case of 'port is linked' state
//
kern_return_t VSPSerialPort::sendToPortLink(void* context, const void* buffer, const uint64_t size)
{
    TResponseActionInfo* ctx = (TResponseActionInfo*)context;

    if (!ctx->self) {
        VSPErr(LOG_PREFIX, "sendToPortLink: Context parameter is invalid.\n");
        return kIOReturnBadArgument;
    }

    IOReturn ret;
    TVSPLinkItem item = {};
    VSPSerialPort* port = nullptr;
    
    uint8_t myLinkId = ivars->m_portLinkId;
    uint8_t myPortId = ivars->m_portId;

    if (traceFlags() & TRACE_PORT_RX) {
        VSPLog(LOG_PREFIX, "sendToPortLink called. using linkId=%d myPortId=%d\n", myLinkId, myPortId);
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
        VSPLog(LOG_PREFIX, "sendToPortLink: Enqueue on targetId=%d\n",
               port->getPortIdentifier());
    }

    if (!port->isConnected()) {
        VSPErr(LOG_PREFIX, "sendToPortLink: Port %d is disconnected, skip\n",
               port->getPortIdentifier());
        return kIOReturnNotOpen;
    }
    
    if ((ret = port->sendResponse(ctx, buffer, size)) != kIOReturnSuccess) {
        return ret;
    }
    
    if (traceFlags() & TRACE_PORT_RX) {
        VSPLog(LOG_PREFIX, "sendToPortLink complete.\n");
    }
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// Called by TxDataAvailable() or sendToPortLink() to dispatch RX data
//
kern_return_t VSPSerialPort::sendResponse(void* context, const void* buffer, const uint64_t sizeIn)
{
    //TResponseActionInfo* ctx = (TResponseActionInfo*)context;
    
    if (traceFlags() & TRACE_PORT_RX) {
        VSPLog(LOG_PREFIX, "sendResponse: [IOSPI-RX-0] called with sizeIn=%llu\n", sizeIn);
    }

    // --- Validation checks ---
    if (!buffer || !sizeIn) {
        VSPErr(LOG_PREFIX, "sendResponse: Buffer NULL or size zero.\n");
        return kIOReturnBadArgument;
    }
    if (!isConnected()) {
        VSPErr(LOG_PREFIX, "sendResponse: This port %d is closed.\n", ivars->m_portId);
        return kIOReturnNotOpen;
    }
    
    IOReturn ret = kIOReturnSuccess;
    const uint8_t* src = reinterpret_cast<const uint8_t*>(buffer);
    bool isWrapped = false;
    uint8_t* base = nullptr;
    uint64_t bytesToWrite = sizeIn;
    uint64_t written = 0;
    uint64_t freeSpace;
    uint64_t capacity;
    uint64_t rxPI;
    uint64_t rxCI;

    // --- Get RX buffer info ---
    if (ivars->m_rxqbmd) {
        ret = ivars->m_rxqbmd->GetAddressRange(&ivars->m_rxseg);
        base = reinterpret_cast<uint8_t*>(ivars->m_rxseg.address);
    }
    if (!base || ret != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "sendResponse: Cannot get RX buffer base pointer.\n");
        ret = kIOReturnVMError;
        goto done;
    }

    capacity = ivars->m_rxseg.length;
    if (capacity == 0) {
        VSPErr(LOG_PREFIX, "sendResponse: RX buffer capacity is zero.\n");
        ret = kIOReturnNoSpace;
        goto done;
    }

    rxPI = ivars->m_spi->rxPI;
    rxCI = ivars->m_spi->rxCI;

    if (traceFlags() & TRACE_PORT_RX) {
        VSPLog(LOG_PREFIX, "sendResponse: [IOSPI-RX-1] rxPI=%llu rxCI=%llu capacity=%llu sizeIn=%llu\n",
               rxPI, rxCI, capacity, bytesToWrite);
    }
    
    // --- Validate indices ---
    if (rxPI >= capacity || rxCI >= capacity) {
        VSPErr(LOG_PREFIX, "sendResponse: Index corruption detected! rxPI=%llu rxCI=%llu sizeIn=%llu cap=%llu\n",
               rxPI, rxCI, sizeIn, capacity);
        ivars->m_spi->rxPI = 0;
        ivars->m_spi->rxCI = 0;
        ret = kIOReturnInvalid;
        goto done;
    }

    // --- Calculate available space ---
    if (rxPI < rxCI) {
        freeSpace = rxCI - rxPI;
    } else {
        freeSpace = capacity - rxCI;
    }
    if (freeSpace == 0) {
        if (traceFlags() & TRACE_PORT_RX) {
            VSPErr(LOG_PREFIX, "sendResponse: Buffer full! rxPI=%llu rxCI=%llu cap=%llu\n",
                   rxPI, rxCI, capacity);
        }
        // copy to top of ring buffer
        //memcpy(base, src, sizeIn);
        //ivars->m_spi->rxPI = static_cast<uint32_t>(sizeIn) % capacity;
        //ivars->m_spi->rxCI = 0;
        ret = kIOReturnNoSpace;
        goto done;
    }

    //bytesToWrite = MIN(bytesToWrite, freeSpace);

    if (traceFlags() & TRACE_PORT_RX) {
        VSPLog(LOG_PREFIX, "sendResponse: [IOSPI-RX-2] rxPI=%llu rxCI=%llu free=%llu toWrite=%llu cap=%llu\n",
               rxPI, rxCI, freeSpace, bytesToWrite, capacity);
    }

    // +++ Copy data to ring buffer +++
    
    // Linear copy ...
    if ((rxPI + bytesToWrite) <= capacity) {
        if (traceFlags() & TRACE_PORT_RX) {
            VSPLog(LOG_PREFIX, "sendResponse: [IOSPI-RX-L] rxPI=%llu rxCI=%llu toWrite=%llu cap=%llu\n",
                   rxPI, rxCI, bytesToWrite, capacity);
        }
        
        memcpy(base + rxPI, src, bytesToWrite);
        
        if ((traceFlags() & TRACE_PORT_RX) /*&& (traceFlags() & TRACE_PORT_IO)*/) {
            dbg_dump_buffer(base + rxPI, bytesToWrite);
        }
        
        written = bytesToWrite;
        rxPI   += written;
    }
    // Wrap-around copy
    else {
        uint64_t firstChunk  = capacity - rxPI;
        uint64_t secondChunk = bytesToWrite - firstChunk;
        
        if (traceFlags() & TRACE_PORT_RX) {
            VSPLog(LOG_PREFIX, "sendResponse: [IOSPI-RX-W] rxPI=%llu rxCI=%llu FIRST=%llu SECOND=%llu\n",
                   rxPI, rxCI, firstChunk, secondChunk);
        }

        // Copy first chunk to end of buffer
        memcpy(base + rxPI, src, firstChunk);
        
        if ((traceFlags() & TRACE_PORT_RX) /*&& (traceFlags() & TRACE_PORT_IO)*/) {
            dbg_dump_buffer(base + rxPI, firstChunk);
        }

        // Copy second chunk to beginning of ring buffer
        if (secondChunk > 0) {
            // Copy second chunk to top of ring buffer
            memcpy(base, src + firstChunk, secondChunk);
            isWrapped = true;

            if ((traceFlags() & TRACE_PORT_RX) /*&& (traceFlags() & TRACE_PORT_IO)*/) {
                dbg_dump_buffer(base, secondChunk);
            }
        }
        
        written = firstChunk + secondChunk;
        rxPI   += written;
    }

    // --- Update producer index ---
    if (!isWrapped) {
        ivars->m_spi->rxPI = static_cast<uint32_t>(rxPI) % capacity;
    }
    else {
        ivars->m_spi->rxPI = static_cast<uint32_t>(rxPI); // Set xPI over capacity.
    }
    
    if (traceFlags() & TRACE_PORT_RX) {
        VSPLog(LOG_PREFIX, "sendResponse: [IOSPI-RX-4] rxPI=%u rxCI=%u bytesWritten=%llu\n",
               ivars->m_spi->rxPI, ivars->m_spi->rxCI, written);
    }

#if 0
    // last write to ring buffer reached end. Reset consumer index
    // to top of ring buffer. Prevent index corruption.
    if (ivars->m_spi->rxPI == 0 && (rxCI + bytesWritten) >= capacity) {
        if (traceFlags() & TRACE_PORT_RX) {
            VSPLog(LOG_PREFIX, "sendResponse: End of buffer! (RESETTING txCI) rxPI=%u rxCI=%u bytesWritten=%llu\n",
                   ivars->m_spi->rxPI, ivars->m_spi->rxCI, bytesWritten);
        }
        ivars->m_spi->rxCI = 0;
    }
#endif
    
    // --- Notify data availability ---
    if (written > 0) {
        // raise DSR signal
        setDataSetReady(true);
        // notify OS ready to read from ring buffer
        RxDataAvailable();
    }
    
done:
    if (traceFlags() & TRACE_PORT_RX) {
        VSPLog(LOG_PREFIX, "sendResponse: completed with ret=0x%x\n", ret);
    }

    return ret;
}
