// ********************************************************************
// vspsmloader.m - VSPDriver loader model states
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
#import <Foundation/Foundation.h>
#import <vspsmloader.h>

@implementation VSPSmLoader

+ (VSPSmLoaderState)processState:(VSPSmLoaderState)currentState withEvent:(VSPSmLoaderEvent)event {
    switch (currentState) {
        case VSPSmLoaderStateUnknown:
        case VSPSmLoaderStateUnloaded:
            switch (event) {
                case VSPSmLoaderEventDiscoveredUnloaded:
                    return VSPSmLoaderStateUnloaded;
                case VSPSmLoaderEventDiscoveredLoaded:
                    return VSPSmLoaderStateActivated;
                case VSPSmLoaderEventActivationStarted:
                    return VSPSmLoaderStateActivating;
                case VSPSmLoaderEventPromptForApproval:
                case VSPSmLoaderEventActivationFinished:
                case VSPSmLoaderEventActivationFailed:
                    return VSPSmLoaderStateActivationError;
                case VSPSmLoaderEventUninstallStarted:
                case VSPSmLoaderEventUninstallFinished:
                    return VSPSmLoaderStateRemoval;
            }

        case VSPSmLoaderStateActivating:
        case VSPSmLoaderStateNeedsApproval:
            switch (event) {
                case VSPSmLoaderEventActivationStarted:
                    return VSPSmLoaderStateActivating;
                case VSPSmLoaderEventPromptForApproval:
                    return VSPSmLoaderStateNeedsApproval;
                case VSPSmLoaderEventActivationFinished:
                    return VSPSmLoaderStateActivated;
                case VSPSmLoaderEventActivationFailed:
                case VSPSmLoaderEventDiscoveredUnloaded:
                case VSPSmLoaderEventDiscoveredLoaded:
                    return VSPSmLoaderStateActivationError;
                case VSPSmLoaderEventUninstallStarted:
                case VSPSmLoaderEventUninstallFinished:
                    return VSPSmLoaderStateRemoval;
            }

        case VSPSmLoaderStateActivated:
            switch (event) {
                case VSPSmLoaderEventActivationStarted:
                    return VSPSmLoaderStateActivating;
                case VSPSmLoaderEventPromptForApproval:
                case VSPSmLoaderEventActivationFailed:
                case VSPSmLoaderEventDiscoveredUnloaded:
                case VSPSmLoaderEventDiscoveredLoaded:
                    return VSPSmLoaderStateActivationError;
                case VSPSmLoaderEventActivationFinished:
                    return VSPSmLoaderStateActivated;
                case VSPSmLoaderEventUninstallStarted:
                case VSPSmLoaderEventUninstallFinished:
                    return VSPSmLoaderStateRemoval;
            }

        case VSPSmLoaderStateActivationError:
            switch (event) {
                case VSPSmLoaderEventActivationStarted:
                    return VSPSmLoaderStateActivating;
                default:
                    return VSPSmLoaderStateActivationError;
            }

        case VSPSmLoaderStateRemoval:
            switch (event) {
                case VSPSmLoaderEventUninstallStarted:
                    return VSPSmLoaderStateRemoval;
                default:
                    return VSPSmLoaderStateActivationError;
            }
    }
}

@end

