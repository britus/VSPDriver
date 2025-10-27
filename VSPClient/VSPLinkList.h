//
//  VSPLinkList.h
//  VSPClient
//
//  Created by Björn Eschrich on 27.10.25.
//

#ifndef VSPLinkList_h
#define VSPLinkList_h

#import <Foundation/Foundation.h>

NS_SWIFT_NAME(TVSPLinkList)
@interface TVSPLinkList : NSObject

@property (nonatomic) uint8_t count;
@property (nonatomic, strong) NSArray<NSNumber *> *links;

- (instancetype)initWithCount:(uint8_t)count
                        links:(NSArray<NSNumber *> *)links;

@end


#endif /* VSPLinkList_h */
