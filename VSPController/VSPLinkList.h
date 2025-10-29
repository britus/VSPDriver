// ********************************************************************
// VSPClient native OS Bridge
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
#ifndef VSPLinkList_h
#define VSPLinkList_h

#import <Foundation/Foundation.h>

NS_SWIFT_NAME(TVSPLinkList)
@interface TVSPLinkList : NSObject

@property (nonatomic) uint8_t count;
@property (nonatomic, strong) NSArray<NSNumber *> *links;

- (instancetype)initWithCount:(uint8_t)count
                        links:(NSArray<NSNumber *> *)links;

@end


#endif /* VSPLinkList_h */
