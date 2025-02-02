// ********************************************************************
// VSPDriver - VSPDriverPrivate.cpp
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************

#ifndef VSPDriverPrivate_h
#define VSPDriverPrivate_h

#include <DriverKit/IOLib.h>
#include <DriverKit/IOService.h>
#include <DriverKit/IOTypes.h>
#include <DriverKit/IOMemoryMap.h>
#include <DriverKit/IOMemoryDescriptor.h>
#include <DriverKit/IOBufferMemoryDescriptor.h>
#include <DriverKit/IODispatchQueue.h>
#include <DriverKit/IODispatchSource.h>
#include <DriverKit/IOTimerDispatchSource.h>
#include <DriverKit/OSData.h>
#include <DriverKit/OSString.h>
#include <DriverKit/OSArray.h>

#define TX_BUFFER_SIZE 16384

class VSPDriver;
class VSPDriverPrivate {
public:
    VSPDriverPrivate(VSPDriver* parent);
    virtual ~VSPDriverPrivate();
    
    virtual IOReturn Start(IOService* provider);
    virtual IOReturn Stop(IOService* provider);
    
    virtual void RxFreeSpaceAvailable();
    virtual IOReturn RxDataAvailable();

    virtual void TxFreeSpaceAvailable();
    virtual IOReturn TxDataAvailable();
    
    virtual IOReturn TxPacketsAvailable(OSAction* action);

    virtual IOReturn SetModemStatus(bool cts, bool dsr, bool ri, bool dcd);
    virtual IOReturn RxError(bool overrun, bool gotBreak, bool framingError, bool parityError);
    
    virtual IOReturn HwActivate();
    virtual IOReturn HwDeactivate();
    virtual IOReturn HwResetFIFO(bool tx, bool rx);
    virtual IOReturn HwSendBreak(bool sendBreak);

    virtual IOReturn HwProgramUART(uint32_t baudRate, uint8_t nDataBits, uint8_t nHalfStopBits, uint8_t parity);

    virtual IOReturn HwProgramBaudRate(uint32_t baudRate);
    virtual IOReturn HwProgramMCR(bool dtr, bool rts);
    virtual IOReturn HwGetModemStatus(bool* cts, bool* dsr, bool* ri, bool* dcd);
    virtual IOReturn HwProgramLatencyTimer(uint32_t latency);
    virtual IOReturn HwProgramFlowControl(uint32_t arg, uint8_t xon, uint8_t xoff);

protected:
    inline void CleanupResources();
    virtual IOReturn SetupTTYBaseName();
    virtual IOReturn ConnectDriverQueues();
    virtual IOReturn SetupFIFOBuffers();
    
private:
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
    
    VSPDriver* m_driver;
    IOService* m_provider;
    
    IOBufferMemoryDescriptor *m_itBuffer;   // Interrupt related buffer
    IOMemoryDescriptor *m_txBuffer;         // Transmit buffer
    IOMemoryDescriptor *m_rxBuffer;         // Receive buffer
    OSAction* m_txAction;                   // Async get client TX packets action

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

#endif /* VSPDriverPrivate_h */
