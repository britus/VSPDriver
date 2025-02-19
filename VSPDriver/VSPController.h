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

#ifndef BIT
#define BIT(x) (1 << x)
#endif

// forward
class VSPSerialPort;

typedef struct {
    VSPSerialPort* port;                    // object instance
    uint8_t        id;                      // port item id
    uint64_t       flags;                   // Trace and check flags
} TVSPPortItem;

typedef struct {
    TVSPPortItem sourcePort;          // first port
    TVSPPortItem targetPort;          // second port
    uint8_t         id;                     // link item id
} TVSPLinkItem;

// --------------------------------------------------------
// used by VSPUserClient
// --------------------------------------------------------
namespace VSPController {

typedef enum {
    vspContextPing   = 0x01,
    vspContextPort   = 0x02,
    vspContextResult = 0x03,
    vspContextError  = 0x04,
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
    uint32_t baudRate;
    uint8_t dataBits;
    uint8_t stopBits;
    uint8_t parity;
    uint8_t flowCtrl;
} TVSPPortParameters;

#define MAGIC_CONTROL 0xBE6605250000L
#define MAX_SERIAL_PORTS 16
#define MAX_PORT_LINKS 8

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
        uint8_t list[MAX_SERIAL_PORTS];
    } ports;
    
    /* Available serial port links */
    struct LinkList {
        uint8_t count;
        uint64_t list[MAX_PORT_LINKS];
    } links;
    
} TVSPControllerData;

#ifndef VSP_UCD_SIZE
#define VSP_UCD_SIZE sizeof(TVSPControllerData)
#endif

} /* namespace: VSPController */

#endif /* VSPClientCommands_h */
