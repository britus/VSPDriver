// ********************************************************************
// VSPClient native OS Bridge
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
#ifndef VSPConverter_h
#define VSPConverter_h

#import <Foundation/Foundation.h>
#import "VSPControllerData.h"
#import "VSPPortListItem.h"
#import "VSPControllerStatus.h"
#import "VSPControllerParameter.h"
#import "VSPPortLink.h"
#import "VSPPortList.h"
#import "VSPLinkList.h"

// C++ Structs
#import "VSPDataModel.h"

NS_ASSUME_NONNULL_BEGIN

@interface TVSPConverter : NSObject

/**
 Konvertiert eine C++ TVSPControllerData Struktur in Objective-C TVSPControllerData Klasse.
 @param cppPointer Pointer auf TVSPControllerData (C++ Struct)
 @param size Größe der Struktur (sizeof(TVSPControllerData))
 @return Instanz der Objective-C Klasse TVSPControllerData
 */
+ (TVSPControllerData *)convertFromCPP:(const void *)cppPointer size:(size_t)size;

@end

NS_ASSUME_NONNULL_END


#endif /* VSPConverter_h */
