//
//  VSPControllerData.h
//  VSPClient
//
//  Created by Björn Eschrich on 27.10.25.
//

#ifndef VSPControllerData_h
#define VSPControllerData_h

#import <Foundation/Foundation.h>
#import "VSPUserContext.h"
#import "VSPControlCommand.h"
#import "VSPControllerStatus.h"
#import "VSPControllerParameter.h"
#import "VSPPortList.h"
#import "VSPLinkList.h"

NS_SWIFT_NAME(TVSPControllerData)
@interface TVSPControllerData : NSObject

@property (nonatomic) TVSPUserContext context;
@property (nonatomic) TVSPControlCommand command;
@property (nonatomic, strong) TVSPControllerStatus *status;
@property (nonatomic, strong) TVSPControllerParameter *parameter;
@property (nonatomic, strong) TVSPPortList *ports;
@property (nonatomic, strong) TVSPLinkList *links;

- (instancetype)initWithContext:(TVSPUserContext)context
                        command:(TVSPControlCommand)command
                         status:(TVSPControllerStatus *)status
                      parameter:(TVSPControllerParameter *)parameter
                          ports:(TVSPPortList *)ports
                          links:(TVSPLinkList *)links;

@end

#endif /* VSPControllerData_h */
