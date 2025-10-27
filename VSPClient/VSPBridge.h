//
//  VSPBridge.h
//  VSPClient
//
//  Created by Björn Eschrich on 27.10.25.
//

#ifndef VSPBridge_h
#define VSPBridge_h

#import <Foundation/Foundation.h>
#import "VSPControllerData.h"
#import "VSPConverter.h"

NS_ASSUME_NONNULL_BEGIN

#ifdef __cplusplus
extern "C" {
#endif

/**
 Bridge function callable from C++.
 Converts a C++ TVSPControllerData pointer to Objective-C TVSPControllerData.
 @param pInput Pointer to TVSPControllerData (C++ struct)
 @param size Size of the C++ struct
 */
void ConvertDataFromCPP(const void *pInput, size_t size);

#ifdef __cplusplus
}
#endif

NS_ASSUME_NONNULL_END

#endif /* VSPBridge_h */
