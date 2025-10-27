//
//  VSPPortLink.h
//  VSPClient
//
//  Created by Björn Eschrich on 27.10.25.
//

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
