//
//  VSPBridge.m
//  VSPClient
//
//  Created by Björn Eschrich on 27.10.25.
//

#import <Foundation/Foundation.h>
#import "VSPBridge.h"
 
/**
* C to Swift callbaks
*/
extern void SwiftDataReady(TVSPControllerData* data, int32_t size);

/**
 * Convert driver data structure to ObjectivC class structure
 */
void ConvertDataFromCPP(const void *pInput, size_t size) {
    // Call the Objective-C converter
    TVSPControllerData * data = [TVSPConverter convertFromCPP:pInput size:size];
    if (data != NULL) {
        SwiftDataReady(data, (int32_t) size);
    }
}
