//
//  VSPLinkList.m
//  VSPClient
//
//  Created by Björn Eschrich on 27.10.25.
//

#import <Foundation/Foundation.h>
#import "VSPLinkList.h"

@implementation TVSPLinkList

- (instancetype)init {
    return [self initWithCount:0 links:@[]];
}

- (instancetype)initWithCount:(uint8_t)count
                        links:(NSArray<NSNumber *> *)links {
    self = [super init];
    if (self) {
        _count = count;
        _links = links;
    }
    return self;
}

@end
