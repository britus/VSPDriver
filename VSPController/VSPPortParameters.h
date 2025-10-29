// ********************************************************************
// VSPClient native OS Bridge
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
#ifndef VSPPortParameters_h
#define VSPPortParameters_h

#import <Foundation/Foundation.h>

NS_SWIFT_NAME(TVSPPortParameters)
@interface TVSPPortParameters : NSObject

@property (nonatomic) uint32_t baudRate;
@property (nonatomic) uint8_t dataBits;
@property (nonatomic) uint8_t stopBits;
@property (nonatomic) uint8_t parity;
@property (nonatomic) uint8_t flowCtrl;

- (instancetype)initWithBaudRate:(uint32_t)baudRate
                        dataBits:(uint8_t)dataBits
                        stopBits:(uint8_t)stopBits
                          parity:(uint8_t)parity
                        flowCtrl:(uint8_t)flowCtrl;

@end

#endif /* VSPPortParameters_h */
