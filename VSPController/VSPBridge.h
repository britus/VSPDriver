// ********************************************************************
// VSPClient native OS Bridge
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
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
 * Bridge function callable from C++.
 * Converts a C++ driver data pointer to Objective-C TVSPControllerData.
 * @param pInput Pointer to driver Data (C++ struct)
 * @param size Size of the C++ struct
 */
void ConvertDataFromCPP(const void *pInput, size_t size);

#ifdef __cplusplus
}
#endif

NS_ASSUME_NONNULL_END

#endif /* VSPBridge_h */
