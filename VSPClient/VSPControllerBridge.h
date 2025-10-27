//
//  VSPControllerBridge.h
//  VSPClient
//
//  Created by Björn Eschrich on 27.10.25.
//

#ifndef VSPControllerBridge_h
#define VSPControllerBridge_h

#include <stdio.h>
#include <stdlib.h>
#include <CoreFoundation/CoreFoundation.h>

extern "C" {
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
bool CreatePort(uint8_t baudRate, uint8_t dataBits, uint8_t stopBits, uint8_t parity, uint8_t flowCtrl);

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
bool EnableChecks(uint8_t port, uint64_t flags);

/**
 *
 */
bool EnableTrace(uint8_t port, uint64_t flags);

} // extern "C"
#endif /* VSPControllerBridge_h */
