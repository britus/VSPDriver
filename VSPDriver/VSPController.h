// ********************************************************************
// VSPController.h - VSP controller constants and definitions
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
#ifndef VSPController_h
#define VSPController_h

// --------------------------------------------------------
// used by VSPDriver
// --------------------------------------------------------

// forward
class VSPSerialPort;

typedef struct {
    VSPSerialPort* port;                    // object instance
    uint8_t        id;                      // port item id
    uint64_t       flags;                   // Trace and check flags
} TVSPSerialPortItem;

typedef struct {
    TVSPSerialPortItem sourcePort;          // first port
    TVSPSerialPortItem targetPort;          // second port
    uint8_t         id;                     // link item id
} TVSPPortLinkItem;

// --------------------------------------------------------
// used by VSPUserClient
// --------------------------------------------------------
namespace VSPController {

typedef enum {
    vspContextPing = 1,
    vspContextPort = 2,
    vspContextResult = 3,
    vspContextError = 4,
} TVSPUserContext;

typedef enum {
    vspControlGetStatus,
    vspControlGetPortList,
    vspControlLinkPorts,
    vspControlUnlinkPorts,
    vspControlEnableChecks,
    vspControlEnableTrace,
    // Has to be last
    vspLastCommand,
} TVSPControlCommand;

typedef struct {
    uint8_t sourceId;
    uint8_t targetId;
} TVSPPortLink;

typedef struct {
    /* In whitch context calld */
    TVSPUserContext context;
    /* User client command */
    TVSPControlCommand command;
    /* Command parameters */
    struct Parameter {
        /* command flags */
        uint64_t flags;
        /* port parameters */
        TVSPPortLink portLink;
    } parameter;
    /* Available serial ports */
    struct PortList {
        uint8_t count;
        uint8_t list[16];
    } ports;
    /* Command status response */
    struct Status {
        uint32_t code;
        uint64_t flags;
    } status;
} TVSPControllerData;

#ifndef VSP_UCD_SIZE
#define VSP_UCD_SIZE sizeof(TVSPControllerData)
#endif

} /* namespace: VSPController */

#endif /* VSPClientCommands_h */
