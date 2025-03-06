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
#include <DriverKit/OSData.h>
#include <DriverKit/OSDictionary.h>
#include <DriverKit/OSCollection.h>
#include <DriverKit/OSCollections.h>
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
#define kRxDataQueueName "rxDataQueue"

#ifndef IOLockFreeNULL
#define IOLockFreeNULL(l) { if (NULL != (l)) { IOLockFree(l); (l) = NULL; } }
#endif

#define VSPAquireLock(ivars) \
{ \
++ivars->m_lockLevel; \
VSPLog(LOG_PREFIX, "=> lock level=%d this=0x%llx", ivars->m_lockLevel, (uint64_t) this); \
IOLockLock(ivars->m_lock); \
}

#define VSPUnlock(ivars) \
{ \
VSPLog(LOG_PREFIX, "<= lock level=%d this=0x%llx", ivars->m_lockLevel, (uint64_t) this); \
--ivars->m_lockLevel; \
IOLockUnlock(ivars->m_lock); \
}

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
    
    IOPropertyName m_portSuffix = {};               // the TTY device number
    IOPropertyName m_portBaseName = {};             // the TTY device name 'vsp'
    
    IOLock* m_lock = nullptr;                       // for resource locking
    volatile atomic_int m_lockLevel = 0;
    
    /* OS provided memory descriptors by overridden
     * method ConnectQueues(...) */
    SerialPortInterface* m_spi;                     // OS serial port interface
    
    IOBufferMemoryDescriptor* m_txqbmd;             // VSP TX queue memory descriptor
    IOAddressSegment m_txseg = {};                  // VSP TX buffer segment
    
    IOBufferMemoryDescriptor* m_rxqbmd;             // VSP RX queue memory descriptor
    IOAddressSegment m_rxseg = {};                  // VSP RX buffer segment
     
    // Serial port interface
    uint32_t m_errorState = 0;
    uint32_t m_hwStatus = 0;
    uint32_t m_hwMCR = 0;
    uint32_t m_hwLatency = 25;

    TUartParameters m_uartParams = {};              // set by TTY client and VSPUserClient
    THwFlowControl m_hwFlowControl = {};            // set by TTY client and VSPUserClient

    uint64_t m_paramChecks = 0;                     // flags for TTY parameter checks
    uint64_t m_traceFlags = 0;                      // flags for runtime tracing
    
    bool m_hwActivated = false;                     // set by OS to activate SP hardware
};

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
    IOReturn ret;
    
    VSPLog(LOG_PREFIX, "readTTYProperties called.\n");
    
    ret = getProperty(self, "IOTTYBaseName", self->ivars->m_portBaseName, sizeof(IOPropertyName)-1);
    if (ret != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "readTTYProperties: getProperty(IOTTYBaseName) failed. code=%x", ret);
        goto finish;
    }

    ret = getProperty(self, "IOTTYSuffix", self->ivars->m_portSuffix, sizeof(IOPropertyName)-1);
    if (ret != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "readTTYProperties: getProperty(IOTTYSuffix) failed. code=%x", ret);
        goto finish;
    }

finish:
    return ret;
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
    
    VSPLog(LOG_PREFIX, "Start: register service.\n");
    
    // Register driver instance to IOReg
    if ((ret = RegisterService()) != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "Start: RegisterService failed. code=%x\n", ret);
        goto error_exit;
    }
    
    readTTYProperties(this);
    
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

    IOLockFreeNULL(ivars->m_lock);
}

bool VSPSerialPort::isConnected()
{
    return (ivars->m_txqbmd && ivars->m_rxqbmd && ivars->m_hwActivated);
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
    
    // Convert the base-2 logarithmic size of the buffer or the in_txqmd parameter.
    txcapacity = (size_t) ::pow(2, in_txqlogsz);
    ret = IOBufferMemoryDescriptor::Create(kIOMemoryDirectionIn, txcapacity, 0, &ivars->m_txqbmd);
    if (ret != kIOReturnSuccess || !ivars->m_txqbmd) {
        VSPErr(LOG_PREFIX, "Start: Unable to create TX memory descriptor. code=%x\n", ret);
        goto error_exit;
    }
    
    // Convert the base-2 logarithmic size of the buffer for the in_rxqmd parameter.
    rxcapacity = (size_t) ::pow(2, in_rxqlogsz);
    ret = IOBufferMemoryDescriptor::Create(kIOMemoryDirectionOut, rxcapacity, 0, &ivars->m_rxqbmd);
    if (ret != kIOReturnSuccess || !ivars->m_rxqbmd) {
        VSPErr(LOG_PREFIX, "Start: Unable to create RX memory descriptor. code=%x\n", ret);
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
    if (ret != kIOReturnSuccess || !ivars->m_txseg.address || ivars->m_txseg.length != txcapacity) {
        VSPErr(LOG_PREFIX, "ConnectQueues: Unable to get TX-MD segment. code=%x\n", ret);
        goto error_exit;
    }
    
    // Get the address segment of the RX memory descriptor
    ret = ivars->m_rxqbmd->GetAddressRange(&ivars->m_rxseg);
    if (ret != kIOReturnSuccess || !ivars->m_rxseg.address || ivars->m_rxseg.length != rxcapacity) {
        VSPErr(LOG_PREFIX, "ConnectQueues: Unable to get RX-MD segment. code=%x\n", ret);
        goto error_exit;
    }
    
    // Get the address segment of the SerialPortInterface (mapped space)
    if ((ret = (*ifmd)->GetAddressRange(&ifseg)) != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "ConnectQueues: IF GetAddressRange failed. code=%x\n", ret);
        goto error_exit;
    }
    
    // Initialize the SPI indexes
    ivars->m_spi = reinterpret_cast<SerialPortInterface*>(ifseg.address);
    ivars->m_spi->txCI = 0;
    ivars->m_spi->txPI = 0;
        
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
  
    // stopping...
    ivars->m_hwActivated = false;

    // Reset modem status
    setDataCarrierDetect(false);
    setDataSetReady(false);
    setClearToSend(false);
    HwDeactivate();

    // reset SPI pointer from OS
    ivars->m_spi = nullptr;
    
    // reset our TX/RX segments
    ivars->m_txseg = {};
    ivars->m_rxseg = {};

    // Remove our memory descriptors
    OSSafeReleaseNULL(ivars->m_txqbmd);
    OSSafeReleaseNULL(ivars->m_rxqbmd);

    ret = DisconnectQueues(SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "super::DisconnectQueues: failed. code=%x\n", ret);
        return ret;
    }
    
    return ret;
}

// --------------------------------------------------------------------
// TxDataAvailable_Impl()
// TX data ready to read from m_txqbmd segment

static inline void update_txqbmd(struct VSPSerialPort_IVars* ivars)
{
    // We reserve 1K size from the capacity from t_txqbmd.
    // This protects against a buffer overflow by OS calls.
    if (((uint32_t) ivars->m_txseg.length - ivars->m_spi->txPI) < 1024) {
        ivars->m_spi->txPI = 0;
        ivars->m_spi->txCI = 0;
    }
    // Set TX consumer index to end of received block. This increases
    // the offset for the m_txqbmd buffer.
    else {
        ivars->m_spi->txCI = ivars->m_spi->txPI;
    }
}

void IMPL(VSPSerialPort, TxDataAvailable)
{
    IOReturn ret = kIOReturnSuccess;
    uint8_t* buffer;
    uint64_t address;
    uint32_t size;
    
    VSPLog(LOG_PREFIX, "--------------------------------------------------\n");
    VSPLog(LOG_PREFIX, "TxDataAvailable called.\n");

    // Lock to ensure thread safety
    VSPAquireLock(ivars);
    
    // We are working
    setClearToSend(false);

    // Show me indexes be fore manipulate
    VSPLog(LOG_PREFIX, "TxDataAvailable: [IOSPI-TX 1] txPI: %d, txCI: %d, txqoffset: %d, txqlogsz: %d",
           ivars->m_spi->txPI, ivars->m_spi->txCI, ivars->m_spi->txqoffset, ivars->m_spi->txqlogsz);
    
    // skip if nothing to do
    if (!ivars->m_spi->txPI) {
        VSPErr(LOG_PREFIX, "TxDataAvailable: spi->txPI is zero, skip\n");
        goto finish;
    }

    // Get address of new TX data position
    address = ivars->m_txseg.address + ivars->m_spi->txCI;
    buffer  = reinterpret_cast<uint8_t*>(address);
    
    // Calculate available data in TX buffer
    size = ivars->m_spi->txPI - ivars->m_spi->txCI;

#ifdef DEBUG // !! Debug ....
    VSPLog(LOG_PREFIX, "TxDataAvailable: dump buffer=0x%llx len=%u\n", (uint64_t) buffer, size);
    
    for (uint64_t i = 0; i < size; i++) {
        VSPLog(LOG_PREFIX, "TxDataAvailable: buffer[%02lld]=0x%02x %c\n", i, buffer[i], buffer[i]);
    }
#endif
    
    // Is this port assigned to a port link?
    if (ivars->m_portLinkId) {
        // send TX to other port instance
        ret = sendToPortLink(buffer, size);
        if (ret != kIOReturnSuccess) {
            VSPErr(LOG_PREFIX, "TxDataAvailable: Data routing failed. code=%x\n", ret);
            goto not_routed; // update spi::tx, no echo. strictly required for OS
        }
    }
    // Loopback TX data
    else {
        ret = sendResponse(this, buffer, size);
        if (ret != kIOReturnSuccess) {
            VSPErr(LOG_PREFIX, "TxDataAvailable: Unable to enqueue response. code=%x\n", ret);
            goto finish;
        }
    }
    
not_routed:

    // update TX indexes in serial port interface
    update_txqbmd(ivars);

    // Show me indexes
    VSPLog(LOG_PREFIX, "TxDataAvailable: [IOSPI-TX 2] txPI: %d, txCI: %d, txqoffset: %d, txqlogsz: %d",
           ivars->m_spi->txPI, ivars->m_spi->txCI, ivars->m_spi->txqoffset, ivars->m_spi->txqlogsz);

    // Update modem status
    setClearToSend(true);
    
    // reset memory
    memset(buffer, 0, size);
    
    VSPLog(LOG_PREFIX, "TxDataAvailable complete.\n");
    
finish:
    VSPUnlock(ivars);
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
    RxFreeSpaceAvailable(SUPERDISPATCH);
}

// --------------------------------------------------------------------
// TxFreeSpaceAvailable_Impl()
// Notify OS ready for more client data
void IMPL(VSPSerialPort, TxFreeSpaceAvailable)
{
    VSPLog(LOG_PREFIX, "TxFreeSpaceAvailable called.\n");
    TxFreeSpaceAvailable(SUPERDISPATCH);
}

// --------------------------------------------------------------------
// Enable/Disable modem status clear to send
//
void VSPSerialPort::setClearToSend(bool cts)
{
    if (IS_BIT(ivars->m_hwStatus, MODEM_STATUS_CTS) != cts) {
        UPDATE_BIT(ivars->m_hwStatus, MODEM_STATUS_CTS, cts);
        reportModemStatus();
    }
}

// --------------------------------------------------------------------
// Enable/Disable modem status data set ready
//
void VSPSerialPort::setDataSetReady(bool dsr)
{
    if (IS_BIT(ivars->m_hwStatus, MODEM_STATUS_DSR) != dsr) {
        UPDATE_BIT(ivars->m_hwStatus, MODEM_STATUS_DSR, dsr);
        reportModemStatus();
    }
}

// --------------------------------------------------------------------
// Enable/Disable modem status ring indicator
//
void VSPSerialPort::setRingIndicator(bool ri)
{
    if (IS_BIT(ivars->m_hwStatus, MODEM_STATUS_RI) != ri) {
        UPDATE_BIT(ivars->m_hwStatus, MODEM_STATUS_RI, ri);
        reportModemStatus();
    }
}

// --------------------------------------------------------------------
// Enable/Disable modem status data carrier detect
//
void VSPSerialPort::setDataCarrierDetect(bool dcd)
{
    if (IS_BIT(ivars->m_hwStatus, MODEM_STATUS_DCD) != dcd) {
        UPDATE_BIT(ivars->m_hwStatus, MODEM_STATUS_DCD, dcd);
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
    
    VSPLog(LOG_PREFIX, "SetModemStatus called [in] CTS=%d DSR=%d RI=%d DCD=%d\n",
           cts, dsr, ri, dcd);
    
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
    kern_return_t ret = kIOReturnIOError;
    
    VSPLog(LOG_PREFIX, "HwActivate called.\n");
    
    VSPAquireLock(ivars);
    ivars->m_hwActivated = true;
    setDataCarrierDetect(true);
    VSPUnlock(ivars);
    
    ret = HwActivate(SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "super::HwActivate failed. code=%x\n", ret);
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
    setDataCarrierDetect(false);
    VSPUnlock(ivars);
    
    ret = HwDeactivate(SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "super::HwDeactivate failed. code=%x\n", ret);
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
    uint64_t len;
    void* buf;
    
    VSPLog(LOG_PREFIX, "HwResetFIFO called -> tx=%d rx=%d\n",
           tx, rx);
    
    VSPAquireLock(ivars);
    if (tx && ivars->m_rxqbmd && ivars->m_spi) {
        ivars->m_spi->txCI = 0;
        ivars->m_spi->txPI = 0;
        buf = (void*) ivars->m_txseg.address;
        len = ivars->m_txseg.length;
        memset(buf, 0, len);
    }
    if (rx && ivars->m_rxqbmd && ivars->m_spi) {
        ivars->m_spi->rxCI = 0;
        ivars->m_spi->rxPI = 0;
        buf = (void*) ivars->m_rxseg.address;
        len = ivars->m_rxseg.length;
        memset(buf, 0, len);
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
    UPDATE_BIT(ivars->m_errorState, ERROR_STATE_BREAK, sendBreak);
    VSPUnlock(ivars);
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// HwGetModemStatus_Impl()
// Called during client communication life cycle
kern_return_t IMPL(VSPSerialPort, HwGetModemStatus)
{
    VSPLog(LOG_PREFIX, "HwGetModemStatus called [out] CTS=%d DSR=%d RI=%d DCD=%d\n", //
           IS_BIT(ivars->m_hwStatus, MODEM_STATUS_CTS),
           IS_BIT(ivars->m_hwStatus, MODEM_STATUS_DSR),
           IS_BIT(ivars->m_hwStatus, MODEM_STATUS_RI),
           IS_BIT(ivars->m_hwStatus, MODEM_STATUS_DCD));
    
    VSPAquireLock(ivars);
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
void VSPSerialPort::setPortItem(VSPDriver* parent, void* data)
{
    if (!parent || !data) {
        return;
    }
    
    // Save instance for VSP controller
    TVSPPortItem* item = (TVSPPortItem*) data;
    ivars->m_parent = parent;
    item->id        = ivars->m_portId;
    item->port      = this;
    item->flags     = 0x00;
}

// --------------------------------------------------------------------
// Remove link to VSPDriver instance
//
void VSPSerialPort::unlinkParent()
{
    VSPLog(LOG_PREFIX, "unlinkParent called.\n");
    
    ivars->m_parent = nullptr;
}

// --------------------------------------------------------------------
// Set the serial port identifier
//
void VSPSerialPort::setPortIdentifier(uint8_t id)
{
    VSPLog(LOG_PREFIX, "setPortIdentifier id=%d.\n", id);
    
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
    VSPLog(LOG_PREFIX, "setPortLinkIdentifier id=%d at port=%d\n",
           id, ivars->m_portId);
    ivars->m_portLinkId = id;
}

// --------------------------------------------------------------------
// Get the serial port link identifier
//
uint8_t VSPSerialPort::getPortLinkIdentifier()
{
    return ivars->m_portLinkId;
}

// --------------------------------------------------------------------
// Enable or disable traces
//
void VSPSerialPort::setTraceFlags(uint64_t flags)
{
    ivars->m_traceFlags = flags;
}

// --------------------------------------------------------------------
// Enable or disable port parameter checks
//
void VSPSerialPort::setParameterChecks(uint64_t flags)
{
    ivars->m_paramChecks = flags;
}

// --------------------------------------------------------------------
// Called by TxDataAvailable in case of 'port is linked' state
//
kern_return_t VSPSerialPort::sendToPortLink(const void* buffer, const uint32_t size)
{
    IOReturn ret;
    TVSPLinkItem item = {};
    VSPSerialPort* port = nullptr;
    uint8_t myid = ivars->m_portLinkId;

    VSPLog(LOG_PREFIX, "sendToPortLink called. using linkId=%d\n", myid);

    ret = ivars->m_parent->getPortLinkById(myid, &item, sizeof(TVSPLinkItem));
    if (ret != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "sendToPortLink: Parent getPortLinkById failed. code=%x\n", ret);
        return ret;
    }

    VSPLog(LOG_PREFIX, "sendToPortLink: got port link.\n");

    if (item.source.id != ivars->m_portId) {
        port = item.source.port;
    }
    else if (item.target.id != ivars->m_portId) {
        port = item.target.port;
    }
    else {
        VSPErr(LOG_PREFIX, "sendToPortLink: Double port IDs detectd! myLinkId=%d myPortId=%d\n",
               myid, ivars->m_portId);
        return kIOReturnInvalid;
    }
    if (!port) {
        VSPErr(LOG_PREFIX, "sendToPortLink: Linked port NULL pointer! myLinkId=%d myPortId=%d\n",
               myid, ivars->m_portId);
        return kIOReturnInvalid;
    }
   
    VSPLog(LOG_PREFIX, "sendToPortLink: got portId=%d -> enqueue on target\n",
           port->getPortIdentifier());

    if (!port->isConnected()) {
        VSPLog(LOG_PREFIX, "sendToPortLink: port is disconnected, skip\n");
        return kIOReturnNotOpen;
    }
    
    if ((ret = port->sendResponse(this, buffer, size)) != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "sendToPortLink: Port %d enqueueResponse failed. code=%x\n",
               ivars->m_portId, ret);
        return ret;
    }
    
    VSPLog(LOG_PREFIX, "sendToPortLink complete.\n");
    return kIOReturnSuccess;
}


// --------------------------------------------------------------------
// Called by TxDataAvailable() or sendToPortLink() to dispatch RX data
//
kern_return_t VSPSerialPort::sendResponse(void* sender, const void* buffer, const uint32_t size)
{
    uint64_t address;
    uint8_t* mdbuffer;
    
    if (!ivars->m_spi || !ivars->m_rxqbmd || !ivars->m_txqbmd || !ivars->m_hwActivated) {
        VSPErr(LOG_PREFIX, "sendResponse: Device is closed.\n");
        return kIOReturnNotOpen;
    }

    // Update modem status
    setDataSetReady(false);
    
    VSPLog(LOG_PREFIX, "sendResponse: [IOSPI-RX 1] rxPI=%d rxCI=%d rxqoffset=%d rxqlogsz=%d\n",
           ivars->m_spi->rxPI, ivars->m_spi->rxCI, ivars->m_spi->rxqoffset, ivars->m_spi->rxqlogsz);
    
    // reset [!! 1 !!]
    // We start always our response from beginning
    // of the memory descriptor buffer
    ivars->m_spi->rxPI = 0;
    ivars->m_spi->rxCI = 0;
    
    // Get address to the RX ring buffer
    address = ivars->m_rxseg.address + ivars->m_spi->rxPI;
    mdbuffer = reinterpret_cast<uint8_t*>(address);
    memcpy(mdbuffer, buffer, size);
    
#ifdef DEBUG // !! Debug ....
    VSPLog(LOG_PREFIX, "sendResponse: Dump m_rxqmd buffer=0x%llx size=%u\n",
           (uint64_t) mdbuffer, size);
    
    for (uint64_t i = 0; i < size; i++) {
        VSPLog(LOG_PREFIX, "sendResponse: buffer[%02lld]=0x%02x %c\n", i,
               mdbuffer[i], mdbuffer[i]);
    }
#endif
    
    // Update RX producer index
    ivars->m_spi->rxPI = size;
    
    VSPLog(LOG_PREFIX, "sendResponse: [IOSPI-RX 2] rxPI=%d rxCI=%d rxqoffset=%d rxqlogsz=%d\n",
           ivars->m_spi->rxPI, ivars->m_spi->rxCI, ivars->m_spi->rxqoffset, ivars->m_spi->rxqlogsz);
    
    // Update modem status
    setDataSetReady(true);
    
    // Notify OS interrest parties
    this->RxDataAvailable_Impl();

    VSPLog(LOG_PREFIX, "sendResponse: complete.\n");
    return kIOReturnSuccess;
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
