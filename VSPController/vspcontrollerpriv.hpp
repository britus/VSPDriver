// ********************************************************************
// VSPController.cpp - VSPDriver user client controller (private)
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************

#include "vspcontroller.hpp"
#include <CoreFoundation/CFNotificationCenter.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOTypes.h>

namespace VSPClient {

class VSPController;

/* The classes below are not exported */
#pragma GCC visibility push(hidden)

class VSPControllerPriv
{
public:
    friend class VSPController;
    explicit VSPControllerPriv(const char* dextClassName, VSPController* parent);
    virtual ~VSPControllerPriv();
    bool ConnectDriver();
    bool GetStatus();
    bool IsConnected();
    bool CreatePort(TVSPPortParameters* parameters);
    bool RemovePort(const uint8_t id);
    bool GetPortList();
    bool GetLinkList();
    bool LinkPorts(const uint8_t source, const uint8_t target);
    bool UnlinkPorts(const uint8_t source, const uint8_t target);
    bool EnableChecks(const uint8_t port, const uint32_t flags);
    bool EnableTrace(const uint8_t port, const uint32_t flags);
    bool SendData(const TVSPControllerData& data);
    const char* DeviceName() const;
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
    VSPController* m_controller = NULL;
    mach_port_t m_machPort;
    CFRunLoopRef m_runLoop = NULL;
    CFRunLoopSourceRef m_runLoopSource = NULL;
    io_iterator_t m_deviceAddedIter = IO_OBJECT_NULL;
    io_iterator_t m_deviceRemovedIter = IO_OBJECT_NULL;
    IONotificationPortRef m_notificationPort = NULL;
    io_connect_t m_drv = 0;
    io_name_t m_deviceName;
    io_name_t m_devicePath;
    TVSPControllerData* m_vspResponse = NULL; // mapped async buffer

    typedef char TDextIdentifier[128];
    TDextIdentifier m_dextClassName = {};

    // set DEXT driver class name specified by DEXT's Info.plist
    // section IOKitPersonalities
    inline bool SetDextIdentifier(const char* name);
    inline bool UserClientSetup(void* refcon);
    inline void UserClientTeardown(void);
    inline bool DoAsyncCall(TVSPControllerData* input);
};

#pragma GCC visibility pop

} // END namespace
