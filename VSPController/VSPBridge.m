// ********************************************************************
// VSPClient native OS Bridge
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
#import <Foundation/Foundation.h>
#import "VSPBridge.h"
 
/**
* C to Swift callbaks
*/
extern void SwiftDataReady(TVSPControllerData* data, int32_t size);
extern void VSPLogMessage(NSString* message);

/**
 * Convert driver data structure to ObjectivC class structure
 */
void ConvertDataFromCPP(const void *pInput, size_t size) {
    TVSPControllerData * data = [TVSPConverter convertFromCPP:pInput size:size];
    if (data != NULL) {
        SwiftDataReady(data, (int32_t) size);
    }
}

/**
 * Send VSP controller log message
 */
void SendLogMessage(const char* buffer, size_t size)
{
    // Convert buffer to NSString and call Swift callback
    NSString *message = buffer ? [NSString stringWithUTF8String:buffer] : @"";
    if (message) {
        VSPLogMessage(message);
    }
}
