// ********************************************************************
// VSPClient native OS Bridge
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
#import <Foundation/Foundation.h>
#import "VSPLinkList.h"

@implementation TVSPLinkList

- (instancetype)init {
    return [self initWithCount:0 links:@[]];
}

- (instancetype)initWithCount:(uint8_t)count
                        links:(NSArray<NSNumber *> *)links {
    self = [super init];
    if (self) {
        _count = count;
        _links = links;
    }
    return self;
}

@end
