// ********************************************************************
// VSPClient native OS Bridge
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
#ifndef VSPControlCommand_h
#define VSPControlCommand_h

#import <Foundation/Foundation.h>

typedef NS_ENUM(uint8_t, TVSPControlCommand) {
    TVSPControlPingPong = 0,
    TVSPControlGetStatus = 1,
    TVSPControlCreatePort = 2,
    TVSPControlRemovePort = 3,
    TVSPControlLinkPorts = 4,
    TVSPControlUnlinkPorts = 5,
    TVSPControlGetPortList = 6,
    TVSPControlGetLinkList = 7,
    TVSPControlEnableChecks = 8,
    TVSPControlEnableTrace = 9,
    TVSPControlShutdown = 10,
    TVSPControlLastCommand = 11
} NS_SWIFT_NAME(TVSPControlCommand);

#endif /* VSPControlCommand_h */
