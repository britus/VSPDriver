// ********************************************************************
// VSPClient native OS Bridge
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
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
