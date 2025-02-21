// ********************************************************************
// VSPLoadModel.h - VSPDriver user setup/install
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
#ifndef VSPLoaderModel_h
#define VSPLoaderModel_h

#import <Foundation/Foundation.h>
#import <SystemExtensions/SystemExtensions.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, VSPSmLoaderState) {
    VSPSmLoaderStateUnknown,
    VSPSmLoaderStateUnloaded,
    VSPSmLoaderStateActivating,
    VSPSmLoaderStateNeedsApproval,
    VSPSmLoaderStateActivated,
    VSPSmLoaderStateActivationError,
    VSPSmLoaderStateRemoval
};

@interface VSPLoaderModel : NSObject <OSSystemExtensionRequestDelegate>

@property (nonatomic, readonly) NSString *dextLoadingState;
@property (nonatomic)   VSPSmLoaderState state;

- (void)activateMyDext;
- (void)removeMyDext;

@end

NS_ASSUME_NONNULL_END

#endif /* VSPLoaderModel_h */
