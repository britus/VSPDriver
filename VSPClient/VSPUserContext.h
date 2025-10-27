//
//  VSPUserContext.h
//  VSPClient
//
//  Created by Björn Eschrich on 27.10.25.
//

#ifndef VSPUserContext_h
#define VSPUserContext_h

#import <Foundation/Foundation.h>

typedef NS_ENUM(uint8_t, TVSPUserContext) {
    TVSPUserContextPing = 0x01,
    TVSPUserContextPort = 0x02,
    TVSPUserContextResult = 0x03,
    TVSPUserContextError = 0x04
} NS_SWIFT_NAME(TVSPUserContext);

#endif /* VSPUserContext_h */
