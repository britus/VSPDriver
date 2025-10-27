//
//  VSPControllerData.m
//  VSPClient
//
//  Created by Björn Eschrich on 27.10.25.
//

#import <Foundation/Foundation.h>
#import "VSPControllerData.h"

@implementation TVSPControllerData

- (instancetype)init {
    return [self initWithContext:TVSPUserContextPing
                         command:TVSPControlPingPong
                          status:[[TVSPControllerStatus alloc] init]
                       parameter:[[TVSPControllerParameter alloc] init]
                           ports:[[TVSPPortList alloc] init]
                           links:[[TVSPLinkList alloc] init]];
}

- (instancetype)initWithContext:(TVSPUserContext)context
                        command:(TVSPControlCommand)command
                         status:(TVSPControllerStatus *)status
                      parameter:(TVSPControllerParameter *)parameter
                          ports:(TVSPPortList *)ports
                          links:(TVSPLinkList *)links {
    self = [super init];
    if (self) {
        _context = context;
        _command = command;
        _status = status;
        _parameter = parameter;
        _ports = ports;
        _links = links;
    }
    return self;
}

@end
