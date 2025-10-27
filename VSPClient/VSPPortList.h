//
//  VSPPortList.h
//  VSPClient
//
//  Created by Björn Eschrich on 27.10.25.
//

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
