//
//  VSPDriverPrivate.h
//  VSPDriver
//
//  Created by Björn Eschrich on 30.01.25.
//

#ifndef VSPDriverPrivate_h
#define VSPDriverPrivate_h

#include <DriverKit/IOService.iig>

class VSPDriver;
class VSPDriverPrivate {
  
public:
    VSPDriverPrivate(VSPDriver* parent);
    virtual ~VSPDriverPrivate();
    
    virtual IOReturn Start(IOService* provider);
    virtual IOReturn Stop(IOService* provider);
    
private:
    VSPDriver* m_driver;
    IOService* m_provider;
    
    IOBufferMemoryDescriptor *m_txBuffer; // Transmit buffer
    IOBufferMemoryDescriptor *m_rxBuffer; // Receive buffer
    IODispatchQueue* m_txQueue;
    IODispatchQueue* m_rxQueue;
    
    // TCP socket connection details
    OSString *m_serverAddress;
    uint16_t m_serverPort;
    bool m_isConnected;
};

#endif /* VSPDriverPrivate_h */
