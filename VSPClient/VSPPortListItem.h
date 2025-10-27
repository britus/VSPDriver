//
//  VSPPortListItem.h
//  VSPClient
//
//  Created by Björn Eschrich on 27.10.25.
//

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
