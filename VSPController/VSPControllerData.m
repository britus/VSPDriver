// ********************************************************************
// VSPClient native OS Bridge
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
#import <Foundation/Foundation.h>
#import "VSPControllerData.h"

@implementation TVSPControllerData

- (instancetype)init {
    return [self initWithContext:TVSPUserContextPing
                         command:TVSPControlPingPong
                      traceFlags:0
                      checkFlags:0
                          status:[[TVSPControllerStatus alloc] init]
                       parameter:[[TVSPControllerParameter alloc] init]
                           ports:[[TVSPPortList alloc] init]
                           links:[[TVSPLinkList alloc] init]];
}

- (instancetype)initWithContext:(TVSPUserContext)context
                        command:(TVSPControlCommand)command
                     traceFlags:(UInt64)traces
                     checkFlags:(UInt64)checks
                         status:(TVSPControllerStatus *)status
                      parameter:(TVSPControllerParameter *)parameter
                          ports:(TVSPPortList *)ports
                          links:(TVSPLinkList *)links {
    self = [super init];
    if (self) {
        _context = context;
        _command = command;
        _traceFlags = traces;
        _checkFlags = checks;
        _status = status;
        _parameter = parameter;
        _ports = ports;
        _links = links;
    }
    return self;
}

@end
