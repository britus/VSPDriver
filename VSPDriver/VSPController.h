// ********************************************************************
// VSPController.h - VSP controller constants and definitions
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
#ifndef VSPController_h
#define VSPController_h

namespace VSPController {

enum VSPUserContext {
    vspContextPing = 1,
    vspContextPort = 2,
};

enum VSPControlCommand {
    vspControlSetCallback = 1,
    vspControlGetPortList = 2,
    vspControlLinkPorts = 3,
    vspControlUnlinkPorts = 4,
    
    // Has to be last
    vspLastCommand,
};

typedef struct {
    uint8_t sourceId;
    uint8_t targetId;
} TPortLink;

#ifndef VSP_UCD_MESSAGE_SIZE
#define VSP_UCD_MESSAGE_SIZE 127
#endif

typedef struct {
    /* In whitch context calld */
    VSPUserContext context;
    /* Command status response */
    struct UCStatus {
        uint32_t code;
        char     message[VSP_UCD_MESSAGE_SIZE + 1];
    } status;
    /* User client command */
    VSPControlCommand command;
    /* Command parameters */
    union Parameter {
        /* Command parameters */
        TPortLink portLink;
    } parameter;
} TVSPControllerData;

#ifndef VSP_UCD_SIZE
#define VSP_UCD_SIZE sizeof(TVSPControllerData)
#endif

} /* namespace: VSPController */

#endif /* VSPClientCommands_h */
