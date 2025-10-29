// ********************************************************************
// VSPClient native OS Bridge
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
#ifndef VSPPortListItem_h
#define VSPPortListItem_h

#import <Foundation/Foundation.h>

NS_SWIFT_NAME(TVSPPortListItem)
@interface TVSPPortListItem : NSObject

@property (nonatomic) uint8_t portId;
@property (nonatomic) uint64_t flags;
@property (nonatomic, strong) NSString *name;

- (instancetype)initWithPortId:(uint8_t)portId
                         flags:(uint64_t)flags
                          name:(NSString *)name;

@end

#endif /* VSPPortListItem_h */
