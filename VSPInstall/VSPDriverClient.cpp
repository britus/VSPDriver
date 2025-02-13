// ********************************************************************
// VSPController.cpp - VSP controller client
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
#include <stdio.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>

extern "C" {
extern void SwiftAsyncCallback(void* refcon, IOReturn result, void** args, UInt32 numArgs);
extern void SwiftDeviceAdded(void* refcon, io_connect_t connection);
extern void SwiftDeviceRemoved(void* refcon);
}

#include "../VSPController/VSPController.hpp"
class VSPDriverClient: public VSPClient::VSPController {
public:
    explicit VSPDriverClient(void* refcon): VSPController(), m_refcon(refcon){};
protected:
    void OnConnected() override {
        m_connected = true;
        SwiftDeviceAdded(m_refcon, GetConnection());
    }
    void OnDisconnected() override {
        m_connected = false;
        SwiftDeviceRemoved(m_refcon);
    }
    void OnIOUCCallback(int result, void** args, uint32_t numArgs) override {
        SwiftAsyncCallback(m_refcon, result, args, numArgs);
    }
    void OnErrorOccured(int error, const char* message) override {
        SwiftAsyncCallback(m_refcon, kIOReturnError, (void**)&message, (UInt32)strlen(message));
    }
private:
    void* m_refcon;
    bool m_connected;
};

extern "C" {
    static VSPDriverClient* g_client;
    
    // MARK: C Constructors/Destructors
    
    // ----------------------------------------------------------------
    //
    //
    bool UserClientSetup(void* refcon)
    {
        if ((g_client = new VSPDriverClient(refcon)) != NULL) {
            return g_client->ConnectDriver();
        }
        return false;
    }
    
    // ----------------------------------------------------------------
    //
    //
    void UserClientTeardown(void)
    {
        if (g_client) {
            delete g_client;
        }
    }
    
    // MARK: C Controller API

    // ----------------------------------------------------------------
    //
    //
    bool GetPortList(void* refcon, io_connect_t connection)
    {
        if (g_client) {
            return g_client->GetPortList();
        }
        return false;
    }
    
    // ----------------------------------------------------------------
    //
    //
    bool LinkPorts(void* refcon, io_connect_t connection, const uint8_t source, const uint8_t target)
    {
        if (g_client) {
            return g_client->LinkPorts(source, target);
        }
        return false;
    }
    
    // ----------------------------------------------------------------
    //
    //
    bool UnlinkPorts(void* refcon, io_connect_t connection, const uint8_t source, const uint8_t target)
    {
        if (g_client) {
            return g_client->UnlinkPorts(source, target);
        }
        return false;
    }
    
    // ----------------------------------------------------------------
    //
    //
    bool EnableChecks(void* refcon, io_connect_t connection, const uint8_t port)
    {
        if (g_client) {
            return g_client->EnableChecks(port);
        }
        return false;
    }
    
    // ----------------------------------------------------------------
    //
    //
    bool EnableTrace(void* refcon, io_connect_t connection, const uint8_t port)
    {
        if (g_client) {
            return g_client->EnableTrace(port);
        }
        return false;
    }
}
