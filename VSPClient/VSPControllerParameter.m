//
//  VSPControllerParameter.m
//  VSPClient
//
//  Created by Björn Eschrich on 27.10.25.
//

#import <Foundation/Foundation.h>
#import "VSPControllerParameter.h"

@implementation TVSPControllerParameter

- (instancetype)init {
    return [self initWithFlags:0 link:[[TVSPPortLink alloc] init]];
}

- (instancetype)initWithFlags:(uint64_t)flags
                         link:(TVSPPortLink *)link {
    self = [super init];
    if (self) {
        _flags = flags;
        _link = link;
    }
    return self;
}

@end
