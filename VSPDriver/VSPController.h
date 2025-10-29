// ********************************************************************
// VSPController.h - VSP controller constants and definitions
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
#ifndef VSPController_h
#define VSPController_h

#include <DriverKit/OSAction.h>
#include <DriverKit/IOServiceNotificationDispatchSource.h>
#include <DriverKit/IOServiceStateNotificationDispatchSource.h>

// --------------------------------------------------------
// used by VSPDriver
// --------------------------------------------------------

#ifndef BIT
#define BIT(x) (1 << x)
#endif

// forward
class VSPSerialPort;

#define TRACE_PORT_RX   BIT(16)
#define TRACE_PORT_TX   BIT(17)
#define TRACE_PORT_IO   BIT(18)

#define CHECK_BAUD      BIT(19)
#define CHECK_DATA_SIZE BIT(20)
#define CHECK_STOP_BITS BIT(21)
#define CHECK_PARITY    BIT(22)
#define CHECK_FLOWCTRL  BIT(23)

extern "C" {
    typedef struct {
        VSPSerialPort* port;        // object instance
        uint8_t        id;          // port item id
        uint64_t       flags;       // Trace and check flags
    } TVSPPortItem;

    typedef struct {
        TVSPPortItem source;        // first port
        TVSPPortItem target;        // second port
        uint8_t      id;            // link item id
    } TVSPLinkItem;
    
    #pragma pack(push,1)
    typedef struct {
        uint32_t id;                // message identifier
        char     message[1024];     // message buffer
    } TSharedMessage;
    #pragma pack(pop)
}

#define SHARED_QUEUE_ALIGNMENT 1
#define SHARED_QUEUE_SIZE      (64 * 1024)  // 64 KB
#define ENTRY_MAX_PAYLOAD      sizeof(TSharedMessage)

// --------------------------------------------------------
// used by VSPUserClient
// --------------------------------------------------------
namespace VSPController {

#define MAGIC_CONTROL 0xBE6605250000L
#define MAX_SERIAL_PORTS 16
#define MAX_PORT_LINKS 16
#define MAX_PORT_NAME 64

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

typedef struct {
    uint8_t  id;
    uint64_t flags;
    char     name[MAX_PORT_NAME];
} TVSPPortListItem;

typedef struct {
    /* In whitch context calld */
    uint8_t context;
    
    /* User client command */
    uint8_t command;
    
    /* global flags */
    uint64_t traceFlags;
    uint64_t checkFlags;
    
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

#ifndef VSP_UCD_SIZE
#define VSP_UCD_SIZE sizeof(TVSPControllerData)
#endif

} /* namespace: VSPController */

#endif // !VSPController_h
