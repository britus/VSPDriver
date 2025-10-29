// ********************************************************************
// VSPClient native OS Bridge
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
#import <Foundation/Foundation.h>
#import "VSPPortList.h"

@implementation TVSPPortList

- (instancetype)init {
    return [self initWithCount:0 items:@[]];
}

- (instancetype)initWithCount:(uint8_t)count
                        items:(NSArray<TVSPPortListItem *> *)items {
    self = [super init];
    if (self) {
        _count = count;
        _items = items;
    }
    return self;
}

@end
