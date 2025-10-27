//
//  VSPPortListItem..m
//  VSPClient
//
//  Created by Björn Eschrich on 27.10.25.
//

#import <Foundation/Foundation.h>
#import "VSPPortListItem.h"

@implementation TVSPPortListItem

- (instancetype)init {
    return [self initWithPortId:0 flags:0 name:@""];
}

- (instancetype)initWithPortId:(uint8_t)portId
                         flags:(uint64_t)flags
                          name:(NSString *)name {
    self = [super init];
    if (self) {
        _portId = portId;
        _flags = flags;
        _name = name;
    }
    return self;
}

@end
