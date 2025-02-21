// ********************************************************************
// VSPSmLoader.h - VSPDriver user setup/install
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
#ifndef VSPSmLoader_h
#define VSPSmLoader_h

#import <Foundation/Foundation.h>
#import <vsploadermodel.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, VSPSmLoaderEvent) {
    VSPSmLoaderEventDiscoveredUnloaded,
    VSPSmLoaderEventDiscoveredLoaded,
    VSPSmLoaderEventActivationStarted,
    VSPSmLoaderEventUninstallStarted,
    VSPSmLoaderEventPromptForApproval,
    VSPSmLoaderEventActivationFinished,
    VSPSmLoaderEventActivationFailed,
    VSPSmLoaderEventUninstallFinished
};

@interface VSPSmLoader : NSObject

+ (VSPSmLoaderState) processState:(VSPSmLoaderState)currentState
                        withEvent:(VSPSmLoaderEvent)event;

@end

NS_ASSUME_NONNULL_END

#endif /* VSPSmLoader_h */
