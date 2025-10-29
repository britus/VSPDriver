// ********************************************************************
// VSPClient native OS Bridge
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
#ifndef VSPPortLink_h
#define VSPPortLink_h

#import <Foundation/Foundation.h>

NS_SWIFT_NAME(TVSPPortLink)
@interface TVSPPortLink : NSObject

@property (nonatomic) uint8_t source;
@property (nonatomic) uint8_t target;

- (instancetype)initWithSource:(uint8_t)source
                        target:(uint8_t)target;

@end

#endif /* VSPPortLink_h */
