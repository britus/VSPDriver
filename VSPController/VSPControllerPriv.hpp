// ********************************************************************
// VSPController.cpp - VSPDriver user client controller (private)
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************

#include <CoreFoundation/CFNotificationCenter.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOTypes.h>

#define MAGIC_CONTROL    0xBE6605250000L
#define MAX_SERIAL_PORTS 16
#define MAX_PORT_LINKS   8

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

typedef struct {
    uint32_t baudRate;
    uint8_t dataBits;
    uint8_t stopBits;
    uint8_t parity;
    uint8_t flowCtrl;
} TVSPPortParameters;

#ifndef VSP_UCD_SIZE
#define VSP_UCD_SIZE sizeof(TVSPControllerData)
#endif

namespace VSPClient {

class VSPController;

/* The classes below are not exported */
#pragma GCC visibility push(hidden)

class VSPControllerPriv
{
public:
    friend class VSPController;

    /** ----------------------
     *
     */
    VSPControllerPriv(VSPController* parent);
    /** ----------------------
     *
     */
    virtual ~VSPControllerPriv();
    /** ----------------------
     *
     */
    bool ConnectDriver();
    /** ----------------------
     *
     */
    bool GetStatus();
    /** ----------------------
     *
     */
    bool IsConnected();
    /** ----------------------
     *
     */
    bool CreatePort(TVSPPortParameters* parameters);
    /** ----------------------
     *
     */
    bool RemovePort(const uint8_t id);
    /** ----------------------
     *
     */
    bool GetPortList();
    /** ----------------------
     *
     */
    bool GetLinkList();
    /** ----------------------
     *
     */
    bool LinkPorts(const uint8_t source, const uint8_t target);
    /** ----------------------
     *
     */
    bool UnlinkPorts(const uint8_t source, const uint8_t target);
    /** ----------------------
     *
     */
    bool EnableChecks(const uint8_t port);
    /** ----------------------
     *
     */
    bool EnableTrace(const uint8_t port);
    /** ----------------------
     *
     */
    const char* DeviceName() const;
    /** ----------------------
     *
     */
    const char* DevicePath() const;

    // called by static DeviceAdded and DeviceRemoved to set connection object
    void SetConnection(io_connect_t connection);

    // called by static AsyncCallback as result of IOUserClient callback
    void AsyncCallback(IOReturn result, void** args, UInt32 numArgs);

    // called by static DeviceAdded too
    void ReportError(IOReturn error, const char* message);

    // called by static DeviceAdded too
    void SetNameAndPath(const char* name, const char* path);

private:
    // If you don't know what value to use here, it should be identical to the
    // IOUserClass value in your IOKitPersonalities. You can double check
    // by searching with the `ioreg` command in your terminal. It will be of
    // type "IOUserService" not "IOUserServer". The client Info.plist must
    // contain:
    // <key>com.apple.developer.driverkit.userclient-access</key>
    // <array>
    //     <string>VSPDriver</string>
    // </array>
    //
    const char* dextIdentifier = "VSPDriver";

    mach_port_t m_machNotificationPort;
    CFRunLoopRef m_runLoop = NULL;
    CFRunLoopSourceRef m_runLoopSource = NULL;
    io_iterator_t m_deviceAddedIter = IO_OBJECT_NULL;
    io_iterator_t m_deviceRemovedIter = IO_OBJECT_NULL;
    IONotificationPortRef m_notificationPort = NULL;
    io_connect_t m_connection = 0;
    VSPController* m_controller = NULL;
    io_name_t m_deviceName;
    io_name_t m_devicePath;
    TVSPControllerData* m_vspResponse = NULL; // mapped async buffer

    inline bool UserClientSetup(void* refcon);
    inline void UserClientTeardown(void);
    inline bool DoAsyncCall(TVSPControllerData* input);
};

#pragma GCC visibility pop

} // END namespace
