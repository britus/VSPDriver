// ********************************************************************
// VSPClient - serial port access
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
#ifndef SerialPortParameters_h
#define SerialPortParameters_h

#import <Foundation/Foundation.h>

@interface SerialPortParameters : NSObject

@property (nonatomic, assign) NSUInteger baudRate;
@property (nonatomic, assign) NSUInteger dataBits;
@property (nonatomic, assign) NSUInteger stopBits;
@property (nonatomic, assign) NSUInteger parity;
@property (nonatomic, assign) NSUInteger flowCtrl;

- (instancetype)initWith:(NSUInteger)baudRate
                dataBits:(NSUInteger)dataBits
                stopBits:(NSUInteger)stopBits
                  parity:(NSUInteger)parity
                flowCtrl:(NSUInteger)flowCtrl;
@end

#endif /* SerialPortParameters_h */
