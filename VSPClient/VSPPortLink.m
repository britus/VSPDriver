//
//  VSPPortLink.m
//  VSPClient
//
//  Created by Björn Eschrich on 27.10.25.
//

#import <Foundation/Foundation.h>

#import "VSPPortLink.h"

@implementation TVSPPortLink

- (instancetype)init {
    return [self initWithSource:0 target:0];
}

- (instancetype)initWithSource:(uint8_t)source
                        target:(uint8_t)target {
    self = [super init];
    if (self) {
        _source = source;
        _target = target;
    }
    return self;
}

@end
