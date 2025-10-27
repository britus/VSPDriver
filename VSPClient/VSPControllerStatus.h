//
//  VSPControllerStatus.h
//  VSPClient
//
//  Created by Björn Eschrich on 27.10.25.
//

#ifndef VSPControllerStatus_h
#define VSPControllerStatus_h

#import <Foundation/Foundation.h>

NS_SWIFT_NAME(TVSPControllerStatus)
@interface TVSPControllerStatus : NSObject

@property (nonatomic) uint32_t code;
@property (nonatomic) uint64_t flags;

- (instancetype)initWithCode:(uint32_t)code
                       flags:(uint64_t)flags;

@end


#endif /* VSPControllerStatus_h */
