// ********************************************************************
// VSPClient native OS Bridge
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
#ifndef VSPControllerParameter_h
#define VSPControllerParameter_h

#import <Foundation/Foundation.h>
#import "VSPPortLink.h"

NS_SWIFT_NAME(TVSPControllerParameter)
@interface TVSPControllerParameter : NSObject

@property (nonatomic) uint64_t flags;
@property (nonatomic, strong) TVSPPortLink *link;

- (instancetype)initWithFlags:(uint64_t)flags
                         link:(TVSPPortLink *)link;

@end

#endif /* VSPControllerParameter_h */
