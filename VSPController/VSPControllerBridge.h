// ********************************************************************
// VSPClient native OS Bridge
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
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
bool CreatePort(uint32_t baudRate, uint8_t dataBits, uint8_t stopBits, uint8_t parity, uint8_t flowCtrl);

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

/**
 *
 */
bool Shutdown();

} // extern "C"
#endif /* VSPControllerBridge_h */
