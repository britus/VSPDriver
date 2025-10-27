//
//  VSPControllerStatus.m
//  VSPClient
//
//  Created by Björn Eschrich on 27.10.25.
//

#import <Foundation/Foundation.h>
#import "VSPControllerStatus.h"

@implementation TVSPControllerStatus

- (instancetype)init {
    return [self initWithCode:0 flags:0];
}

- (instancetype)initWithCode:(uint32_t)code
                       flags:(uint64_t)flags {
    self = [super init];
    if (self) {
        _code = code;
        _flags = flags;
    }
    return self;
}

@end
