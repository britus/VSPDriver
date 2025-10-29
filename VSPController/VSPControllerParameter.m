// ********************************************************************
// VSPClient native OS Bridge
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
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
