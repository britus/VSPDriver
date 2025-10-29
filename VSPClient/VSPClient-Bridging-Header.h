//
// Use this file to import your target's public
// headers that you would like to expose to Swift.
//
#import <Foundation/Foundation.h>
#import "SerialPortParameters.h"
#import "SerialPort.h"
#import "VSPControllerData.h"
#import "VSPConverter.h"

/**
 *
 */
bool ConnectDriver(void);

/**
 *
 */
void DisconnectDriver(void);

/**
 *
 */
bool GetStatus(void);

/**
 *
 */
bool IsDriverConnected(void);

/**
 *
 */
bool CreatePort(uint32_t baudRate, uint8_t dataBits, uint8_t stopBits,
                uint8_t parity, uint8_t flowCtrl);

/**
 *
 */
bool RemovePort(uint8_t id);

/**
 *
 */
bool GetPortList(void);

/**
 *
 */
bool GetLinkList(void);

/**
 *
 */
bool LinkPorts(uint8_t source, uint8_t target);

/**
 *
 */
bool UnlinkPorts(uint8_t source, uint8_t target);

/**
 *
 */
bool EnableChecksAndTrace(uint8_t port, uint64_t checkFlags, uint64_t traceFlags);
