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
#include <math.h>

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

typedef struct {
    uint8_t* buffer;
    uint32_t length;
} TRXBufferState;

// Driver instance state resource
struct VSPSerialPort_IVars {
    IOService* m_provider = nullptr;
    VSPDriver* m_parent = nullptr;                  // VSPDriver instance
    
    uint8_t m_portId = 0;                           // port id given by VSPDriver
    uint8_t m_portLinkId = 0;                       // port link id given by VSPDriver
    
    IOLock* m_lock = nullptr;                       // for resource locking
    volatile atomic_int m_lockLevel = 0;
    
    /* OS provided memory descriptors by overridden
     * method ConnectQueues(...) */
    SerialPortInterface* m_spi;                     // OS serial port interface
    
    IOBufferMemoryDescriptor* m_txqbmd;             // VSP TX queue memory descriptor
    IOAddressSegment m_txseg = {};                  // VSP TX buffer segment
    
    IOBufferMemoryDescriptor* m_rxqbmd;             // VSP RX queue memory descriptor
    IOAddressSegment m_rxseg = {};                  // VSP RX buffer segment
     
    // Serial interface
    TErrorState m_errorState = {};
    TUartParameters m_uartParams = {};
    THwSerialStatus m_hwStatus = {};
    THwFlowControl m_hwFlowControl = {};
    THwMCR m_hwMCR = {};
    uint32_t m_hwLatency = 25;
    bool m_txNextOffset = 0;
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
    
    VSPLog(LOG_PREFIX, "Start: register service.\n");
    
    // Register driver instance to IOReg
    if ((ret = RegisterService()) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "Start: RegisterService failed. code=%d\n", ret);
        goto error_exit;
    }
    
    VSPLog(LOG_PREFIX, "Start: Port started successfully.\n");
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
    
    // Unlink VSP
    ivars->m_parent = nullptr;
    ivars->m_portId = 0;
    ivars->m_portLinkId = 0;
    
    // Remove all IVars resources
    cleanupResources();
    
    /* Shutdown instane */
    if ((ret= Stop(provider, SUPERDISPATCH)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "super::Stop failed. code=%d\n", ret);
    } else {
        VSPLog(LOG_PREFIX, "Port successfully removed.\n");
    }
    
    return ret;
}

// --------------------------------------------------------------------
// Remove all resources in IVars
//
void VSPSerialPort::cleanupResources()
{
    VSPLog(LOG_PREFIX, "cleanupResources called.\n");
    
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
    IOAddressSegment ifseg = {};
    size_t txcapacity, rxcapacity;
    IOReturn ret;
    
    VSPLog(LOG_PREFIX, "ConnectQueues called\n");
    
    //-- Sane check --//
    if (!in_txqlogsz || !in_rxqlogsz) {
        VSPLog(LOG_PREFIX, "ConnectQueues: Invalid in_rxqlogsz or in_txqlogsz detected.\n");
        return kIOReturnBadArgument;
    }
    
    // Lock to ensure thread safety
    VSPAquireLock(ivars);
    
    // Convert the base-2 logarithmic size of the buffer or the in_txqmd parameter.
    txcapacity = (size_t) ::pow(2, in_txqlogsz);
    ret = IOBufferMemoryDescriptor::Create(kIOMemoryDirectionIn, txcapacity, 0, &ivars->m_txqbmd);
    if (ret != kIOReturnSuccess || !ivars->m_txqbmd) {
        VSPLog(LOG_PREFIX, "Start: Unable to create TX memory descriptor. code=%d\n", ret);
        goto error_exit;
    }
    
    // Convert the base-2 logarithmic size of the buffer for the in_rxqmd parameter.
    rxcapacity = (size_t) ::pow(2, in_rxqlogsz);
    ret = IOBufferMemoryDescriptor::Create(kIOMemoryDirectionOut, rxcapacity, 0, &ivars->m_rxqbmd);
    if (ret != kIOReturnSuccess || !ivars->m_rxqbmd) {
        VSPLog(LOG_PREFIX, "Start: Unable to create RX memory descriptor. code=%d\n", ret);
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
        VSPLog(LOG_PREFIX, "super::ConnectQueues failed. code=%d\n", ret);
        goto error_exit;
    }
    
    //-- Sane check --//
    if (!ifmd || !(*ifmd) || !txqmd || !(*txqmd) || !rxqmd || !(*rxqmd)) {
        VSPLog(LOG_PREFIX, "ConnectQueues: Invalid memory descriptors detected. (NULL)\n");
        ret = kIOReturnBadArgument;
        goto error_exit;
    }
    if ((*txqmd) != ivars->m_txqbmd) {
        VSPLog(LOG_PREFIX, "ConnectQueues: Invalid 'txqmd' memory descriptor detected.\n");
        ret = kIOReturnInvalid;
        goto error_exit;
    }
    if ((*rxqmd) != ivars->m_rxqbmd) {
        VSPLog(LOG_PREFIX, "ConnectQueues: Invalid 'rxqmd' memory descriptor detected.\n");
        ret = kIOReturnInvalid;
        goto error_exit;
    }
    
    // Get the address segment of the TX memory descriptor
    ret = ivars->m_txqbmd->GetAddressRange(&ivars->m_txseg);
    if (ret != kIOReturnSuccess || !ivars->m_txseg.address || ivars->m_txseg.length != txcapacity) {
        VSPLog(LOG_PREFIX, "ConnectQueues: Unable to get TX-MD segment. code=%d\n", ret);
        goto error_exit;
    }
    
    // Get the address segment of the RX memory descriptor
    ret = ivars->m_rxqbmd->GetAddressRange(&ivars->m_rxseg);
    if (ret != kIOReturnSuccess || !ivars->m_rxseg.address || ivars->m_rxseg.length != rxcapacity) {
        VSPLog(LOG_PREFIX, "ConnectQueues: Unable to get TX-MD segment. code=%d\n", ret);
        goto error_exit;
    }
    
    // Get the address segment of the SerialPortInterface (mapped space)
    if ((ret = (*ifmd)->GetAddressRange(&ifseg)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "ConnectQueues: IF GetAddressRange failed. code=%d\n", ret);
        goto error_exit;
    }
    
    // Initialize the indexes
    ivars->m_spi = reinterpret_cast<SerialPortInterface*>(ifseg.address);
    ivars->m_spi->txCI = 0;
    ivars->m_spi->txPI = 0;
        
    // Modem is ready
    ivars->m_hwStatus.dcd = true;
    ivars->m_hwStatus.cts = true;
    
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
    
    // Remove our memory descriptors
    OSSafeReleaseNULL(ivars->m_txqbmd);
    OSSafeReleaseNULL(ivars->m_rxqbmd);
    
    // reset our TX/RX segments
    ivars->m_txseg = {};
    ivars->m_rxseg = {};
    
    // Reset modem status
    ivars->m_hwStatus.dcd = false;
    ivars->m_hwStatus.dsr = false;
    ivars->m_hwStatus.cts = false;
    
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
// TxDataAvailable_Impl()
// TX data ready to read from m_txqbmd segment

static inline void update_txqbmd(struct VSPSerialPort_IVars* ivars)
{
    // We reserve 1K size from the capacity from t_txqbmd.
    // This protects against a buffer overflow.
    if (((uint32_t) ivars->m_txseg.length - ivars->m_spi->txPI) < 1024) {
        ivars->m_spi->txPI = 0;
        ivars->m_spi->txCI = 0;
    }
    // Reset TX consumer index to end of received block. This increases the
    // offset for the m_txqbmd buffer. Finally the DEXT crash if to protection.
    // Reset values like txPI = 0 and txCI = 0 after some transmissions.
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
    
    // We working...
    ivars->m_hwStatus.cts = false;
    ivars->m_hwStatus.dsr = false;
    
    // Show me indexes be fore manipulate
    VSPLog(LOG_PREFIX, "TxDataAvailable: [IOSPI-TX 1] txPI: %d, txCI: %d, txqoffset: %d, txqlogsz: %d",
           ivars->m_spi->txPI, ivars->m_spi->txCI, ivars->m_spi->txqoffset, ivars->m_spi->txqlogsz);
    
    // skip if nothing to do
    if (!ivars->m_spi->txPI) {
        VSPLog(LOG_PREFIX, "TxDataAvailable: spi->txPI is zero, skip\n");
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
    
    // Is port is assigned to a port link?
    if (ivars->m_portLinkId) {
        // send TX to other port instance
        ret = sendToPortLink(buffer, size);
        if (ret != kIOReturnSuccess) {
            VSPLog(LOG_PREFIX, "TxDataAvailable: Data routing failed. code=%d\n", ret);
            goto finish;
        }
    }
    // Loopback TX data
    else {
        ret = sendResponse(this, buffer, size);
        if (ret != kIOReturnSuccess) {
            VSPLog(LOG_PREFIX, "TxDataAvailable: Unable to enqueue response. code=%d\n", ret);
            goto finish;
        }
    }

    // update TX in serial port interface
    update_txqbmd(ivars);

    // Show me indexes be fore manipulation
    VSPLog(LOG_PREFIX, "TxDataAvailable: [IOSPI-TX 2] txPI: %d, txCI: %d, txqoffset: %d, txqlogsz: %d",
           ivars->m_spi->txPI, ivars->m_spi->txCI, ivars->m_spi->txqoffset, ivars->m_spi->txqlogsz);

    // Update modem status
    ivars->m_hwStatus.cts = true;

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
    
    //CreateNameMatchingDictionary(OSString *serviceName, OSDictionary *matching)
    //baseName = OSString::withCString(kVSPTTYBaseName);
    //properties->setObject(kIOTTYBaseNameKey, baseName);
    
    // write back to driver instance
    ret = SetProperties(properties);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "setupTTYBaseName: Unable to set TTY base name. code=%d\n", ret);
        //return ret; // ??? an error - why???
    }
    
    OSSafeReleaseNULL(baseName);
    OSSafeReleaseNULL(properties);
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// Called by TxDataAvailable in case of 'port is linked' state
//
kern_return_t VSPSerialPort::sendToPortLink(const void* buffer, const uint32_t size)
{
    IOReturn ret;
    VSPSerialPort* port = nullptr;
    TVSPLinkItem* item = nullptr;
    uint8_t id = ivars->m_portLinkId;
    void* link = nullptr;

    VSPLog(LOG_PREFIX, "sendToPortLink called. using linkId=%d\n", id);

    if ((ret = ivars->m_parent->getPortLinkById(id, &link)) != kIOReturnSuccess || !link) {
        VSPLog(LOG_PREFIX, "sendToPortLink: Parent getPortLinkById failed. code=%d\n", ret);
        return ret;
    }

    VSPLog(LOG_PREFIX, "sendToPortLink: got port link.\n");

    item = reinterpret_cast<TVSPLinkItem*>(link);
    if (item->sourcePort.id != ivars->m_portId) {
        port = item->sourcePort.port;
    }
    else if (item->targetPort.id != ivars->m_portId) {
        port = item->targetPort.port;
    } else {
        VSPLog(LOG_PREFIX, "sendToPortLink: Double port IDs detectd! myLinkId=%d myPortId=%d\n",
               id, ivars->m_portId);
        return kIOReturnInvalid;
    }
    if (!port) {
        VSPLog(LOG_PREFIX, "sendToPortLink: Linked port NULL pointer! myLinkId=%d myPortId=%d\n",
               id, ivars->m_portId);
        return kIOReturnInvalid;
    }
   
    VSPLog(LOG_PREFIX, "sendToPortLink: got portId=%d -> enqueue on target\n",
           port->getPortIdentifier());

    if ((ret = port->sendResponse(this, buffer, size)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "sendToPortLink: Port %d enqueueResponse failed. code=%d\n",
               ivars->m_portId, ret);
    }
    
    VSPLog(LOG_PREFIX, "sendToPortLink complete.\n");
    
    return ret;
}


// --------------------------------------------------------------------
// Called by TxDataAvailable() or sendToPortLink() to dispatch RX data
//
kern_return_t VSPSerialPort::sendResponse(void* sender, const void* buffer, const uint32_t size)
{
    uint64_t address;
    uint8_t* mdbuffer;
    
    if (!ivars->m_spi || !ivars->m_rxqbmd || !ivars->m_txqbmd || !ivars->m_hwActivated) {
        VSPLog(LOG_PREFIX, "sendResponse: Device is closed.\n");
        return kIOReturnNotOpen;
    }
    
    // Update modem status
    ivars->m_hwStatus.dsr = false;

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
    ivars->m_hwStatus.dsr = true;

    // Notify OS interrest parties
    this->RxDataAvailable_Impl();

    VSPLog(LOG_PREFIX, "sendResponse: complete.\n");

    return kIOReturnSuccess;
}
