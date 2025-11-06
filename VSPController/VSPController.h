// ********************************************************************
// VSPClient native OS Bridge
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
#pragma once
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOTypes.h>
#include "VSPDriverDataModel.h"

class VSPController
{
public:
    explicit VSPController(const char *dextClassName);
    ~VSPController();
    // API
    bool ConnectDriver();
    bool GetStatus();
    bool IsConnected();
    bool CreatePort(CVSPPortParameters *parameters);
    bool RemovePort(const uint8_t id);
    bool GetPortList();
    bool GetLinkList();
    bool LinkPorts(const uint8_t source, const uint8_t target);
    bool UnlinkPorts(const uint8_t source, const uint8_t target);
    bool EnableChecksAndTrace(const uint8_t port, const uint64_t checkFlags, const uint64_t traceFlags);
    bool SendData(const CVSPDriverData &data);
    bool ShutdownDriver();
    // Helper
    const char *deviceName() const;
    const char *devicePath() const;
    // called by static DeviceAdded and DeviceRemoved to set connection object
    void setConnection(io_connect_t connection);
    // called by static AsyncCallback as result of IOUserClient callback
    void asyncCallback(IOReturn result, void **args, UInt32 numArgs);
    // called by static DeviceAdded too
    void reportError(IOReturn error, const char *message);
    // called by static DeviceAdded too
    void setNameAndPath(const char *name, const char *path);
    
private:
    // mapped async buffer
    CVSPDriverData *m_vspResponse;
    CVSPSystemError m_errorInfo;
    
    typedef char TDextIdentifier[128];
    TDextIdentifier m_dextClassName = {};

    inline const CVSPSystemError systemError(int error) const;

    // set DEXT driver class name specified by DEXT's Info.plist
    // section IOKitPersonalities
    inline bool setDextClassName(const char *name);
    // Create DriverKit DEXT IOUserClient instance
    inline bool userClientSetup(void *refcon);
    // Shutdown DriverKit DEXT IOUserClient instance
    inline void userClientTeardown(void);
    // Called by IOConnectCallAsyncStructMethod async callback
    inline bool asyncCall(CVSPDriverData *input);
};
