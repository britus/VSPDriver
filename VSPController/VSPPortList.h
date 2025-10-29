// ********************************************************************
// VSPClient native OS Bridge
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
#ifndef VSPPortList_h
#define VSPPortList_h

#import <Foundation/Foundation.h>
#import "VSPPortListItem.h"

NS_SWIFT_NAME(TVSPPortList)
@interface TVSPPortList : NSObject

@property (nonatomic) uint8_t count;
@property (nonatomic, strong) NSArray<TVSPPortListItem *> *items;

- (instancetype)initWithCount:(uint8_t)count
                        items:(NSArray<TVSPPortListItem *> *)items;

@end


#endif /* VSPPortList_h */
