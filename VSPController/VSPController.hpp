// ********************************************************************
// VSPController.hpp - VSPDriver user client controller
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
#ifndef VSPController_
#define VSPController_

extern "C" {
#define __FAVOR_BSD
#include <inttypes.h>
#include <stdlib.h>
}

/* The classes below are exported */
#pragma GCC visibility push(default)

namespace VSPClient {

class VSPControllerPriv;
class VSPController
{
public:
    VSPController();
    ~VSPController();
    /** ----------------------
     *
     */
    bool ConnectDriver();
    /** ----------------------
     *
     */
    const char* DeviceName() const;
    /** ----------------------
     *
     */
    const char* DevicePath() const;
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
    bool CreatePort();
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
    
protected:
    friend class VSPControllerPriv;
    /** ----------------------
     *
     */
    virtual void OnIOUCCallback(int result, void** args, uint32_t numArgs) = 0;
    /** ----------------------
     *
     */
    virtual void OnConnected() = 0;
    /** ----------------------
     *
     */
    virtual void OnDisconnected() = 0;
    /** ----------------------
     *
     */
    virtual void OnErrorOccured(int error, const char* message) = 0;
    /** ----------------------
     *
     */
    int GetConnection();
    
private:
    VSPControllerPriv *p;
};

} // END namespace

#pragma GCC visibility pop
#endif
