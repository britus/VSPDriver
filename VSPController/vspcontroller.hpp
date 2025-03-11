// ********************************************************************
// VSPController.hpp - VSPDriver user client controller
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
#pragma once

extern "C" {
#define __FAVOR_BSD
#include <stdlib.h>
}

#include "vspcontroller_global.h"

/* The classes below are exported */
#pragma GCC visibility push(default)

namespace VSPClient {

#define MAGIC_CONTROL    0xBE6605250000L
#define MAX_SERIAL_PORTS 16
#define MAX_PORT_LINKS   16
#define MAX_PORT_NAME    64

#ifndef VSP_UCD_SIZE
#define VSP_UCD_SIZE sizeof(TVSPControllerData)
#endif

typedef enum {
    vspContextPing = 0x01,
    vspContextPort = 0x02,
    vspContextResult = 0x03,
    vspContextError = 0x04,
} TVSPUserContext;

typedef enum {
    vspControlPingPong,
    vspControlGetStatus,
    vspControlCreatePort,
    vspControlRemovePort,
    vspControlLinkPorts,
    vspControlUnlinkPorts,
    vspControlGetPortList,
    vspControlGetLinkList,
    vspControlEnableChecks,
    vspControlEnableTrace,
    // Has to be last
    vspLastCommand,
} TVSPControlCommand;

typedef struct {
    uint8_t id;
    char name[MAX_PORT_NAME];
} TVSPPortListItem;

typedef struct {
    uint32_t baudRate;
    uint8_t dataBits;
    uint8_t stopBits;
    uint8_t parity;
    uint8_t flowCtrl;
} TVSPPortParameters;

typedef struct {
    /* In whitch context calld */
    uint8_t context;

    /* User client command */
    uint8_t command;

    /* Command status response */
    struct Status {
        uint32_t code;
        uint64_t flags;
    } status;

    /* Command parameters */
    struct Parameter {
        /* parameter flags */
        uint64_t flags;

        /* port link parameters */
        struct PortLink {
            uint8_t source;
            uint8_t target;
        } link;
    } parameter;

    /* Available serial ports */
    struct PortList {
        uint8_t count;
        TVSPPortListItem list[MAX_SERIAL_PORTS];
    } ports;

    /* Available serial port links */
    struct LinkList {
        uint8_t count;
        uint64_t list[MAX_PORT_LINKS];
    } links;

} TVSPControllerData;

typedef struct {
    int system;
    int sub;
    int code;
} TVSPSystemError;

class VSPControllerPriv;

class VSPCONTROLLER_EXPORT VSPController
{
public:
    explicit VSPController(const char* dextClassName);
    ~VSPController();
    bool ConnectDriver();
    const char* DeviceName() const;
    const char* DevicePath() const;
    bool GetStatus();
    bool IsConnected();
    bool CreatePort(TVSPPortParameters* parameters);
    bool RemovePort(const uint8_t id);
    bool GetPortList();
    bool GetLinkList();
    bool LinkPorts(const uint8_t source, const uint8_t target);
    bool UnlinkPorts(const uint8_t source, const uint8_t target);
    bool EnableChecks(const uint8_t port, const uint32_t flags = 0);
    bool EnableTrace(const uint8_t port, const uint32_t flags = 0);
    bool SetDextIdentifier(const char* name);
    bool SendData(const TVSPControllerData& data);
    const TVSPSystemError GetSystemError(int error) const;

protected:
    friend class VSPControllerPriv;
    virtual int GetConnection();
    virtual void OnIOUCCallback(int result, void* data, uint32_t size) = 0;
    virtual void OnConnected() = 0;
    virtual void OnDisconnected() = 0;
    virtual void OnErrorOccured(const VSPClient::TVSPSystemError& error, const char* message) = 0;
    virtual void OnDataReady(const TVSPControllerData& data) = 0;

private:
    VSPControllerPriv* p;
};

} // END namespace

#pragma GCC visibility pop
