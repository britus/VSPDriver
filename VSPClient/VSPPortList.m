//
//  VSPPortList.m
//  VSPClient
//
//  Created by Björn Eschrich on 27.10.25.
//

#import <Foundation/Foundation.h>
#import "VSPPortList.h"

@implementation TVSPPortList

- (instancetype)init {
    return [self initWithCount:0 items:@[]];
}

- (instancetype)initWithCount:(uint8_t)count
                        items:(NSArray<TVSPPortListItem *> *)items {
    self = [super init];
    if (self) {
        _count = count;
        _items = items;
    }
    return self;
}

@end
