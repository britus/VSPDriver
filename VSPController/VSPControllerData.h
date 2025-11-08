// ********************************************************************
// VSPClient native OS Bridge
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
#ifndef VSPControllerData_h
#define VSPControllerData_h

#import <Foundation/Foundation.h>
#import "VSPUserContext.h"
#import "VSPCommands.h"
#import "VSPControllerStatus.h"
#import "VSPControllerParameter.h"
#import "VSPPortList.h"
#import "VSPLinkList.h"

NS_SWIFT_NAME(TVSPControllerData)
@interface TVSPControllerData : NSObject

@property (nonatomic) TVSPUserContext context;
@property (nonatomic) TVSPControlCommand command;
@property (nonatomic) UInt64 traceFlags;
@property (nonatomic) UInt64 checkFlags;
@property (nonatomic, strong) TVSPControllerStatus *status;
@property (nonatomic, strong) TVSPControllerParameter *parameter;
@property (nonatomic, strong) TVSPPortList *ports;
@property (nonatomic, strong) TVSPLinkList *links;

- (instancetype)initWithContext:(TVSPUserContext)context
                        command:(TVSPControlCommand)command
                     traceFlags:(UInt64)traces
                     checkFlags:(UInt64)checks
                         status:(TVSPControllerStatus *)status
                      parameter:(TVSPControllerParameter *)parameter
                          ports:(TVSPPortList *)ports
                          links:(TVSPLinkList *)links;

@end

#endif /* VSPControllerData_h */
