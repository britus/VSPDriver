//
//  VSPControlCommand.h
//  VSPClient
//
//  Created by Björn Eschrich on 27.10.25.
//

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
    TVSPControlLastCommand = 10
} NS_SWIFT_NAME(TVSPControlCommand);

#endif /* VSPControlCommand_h */
