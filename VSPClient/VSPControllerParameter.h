//
//  VSPControllerParameter.h
//  VSPClient
//
//  Created by Björn Eschrich on 27.10.25.
//

#ifndef VSPControllerParameter_h
#define VSPControllerParameter_h

#import <Foundation/Foundation.h>
#import "VSPPortLink.h"

NS_SWIFT_NAME(TVSPControllerParameter)
@interface TVSPControllerParameter : NSObject

@property (nonatomic) uint64_t flags;
@property (nonatomic, strong) TVSPPortLink *link;

- (instancetype)initWithFlags:(uint64_t)flags
                         link:(TVSPPortLink *)link;

@end

#endif /* VSPControllerParameter_h */
