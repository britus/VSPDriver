// ********************************************************************
// VSPClient - serial port access
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
#import <Foundation/Foundation.h>
#import "SerialPortParameters.h"

@implementation SerialPortParameters

- (instancetype)initWith:(NSUInteger)baudRate
                dataBits:(NSUInteger)dataBits
                stopBits:(NSUInteger)stopBits
                  parity:(NSUInteger)parity
                flowCtrl:(NSUInteger)flowCtrl
{
    self = [super init];
    if (self) {
        _baudRate = baudRate;
        _parity = parity;
        _dataBits = dataBits;
        _stopBits = stopBits;
        _flowCtrl = flowCtrl;
    }
    return self;
}

@end
