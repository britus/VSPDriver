//
//  VSPPortParameters.m
//  VSPClient
//
//  Created by Björn Eschrich on 27.10.25.
//

#import <Foundation/Foundation.h>
#import "VSPPortParameters.h"

@implementation TVSPPortParameters

- (instancetype)init {
    return [self initWithBaudRate:0 dataBits:0 stopBits:0 parity:0 flowCtrl:0];
}

- (instancetype)initWithBaudRate:(uint32_t)baudRate
                        dataBits:(uint8_t)dataBits
                        stopBits:(uint8_t)stopBits
                          parity:(uint8_t)parity
                        flowCtrl:(uint8_t)flowCtrl {
    self = [super init];
    if (self) {
        _baudRate = baudRate;
        _dataBits = dataBits;
        _stopBits = stopBits;
        _parity = parity;
        _flowCtrl = flowCtrl;
    }
    return self;
}

@end
