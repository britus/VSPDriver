// ********************************************************************
// VSPLoadModel.m - VSPDriver user setup/install
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
#import <os/log.h>

#import <Foundation/Foundation.h>
#import <SystemExtensions/SystemExtensions.h>
#import <SystemConfiguration/SystemConfiguration.h>

#import <VSPLoaderModel.h>
#import <VSPSmLoader.h>

void onDidFailWithError(uint32_t code, const char* message);
void onDidFinishWithResult(uint32_t code, const char* message);
void onNeedsUserApproval(void);

@implementation VSPLoaderModel {
    NSString *_dextIdentifier;
    BOOL _isUserUnload;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _dextIdentifier = @"org.eof.tools.VSPDriver";
        _state = VSPSmLoaderStateUnknown;

        // System Extension Properties Check
        if (@available(macOS 12, *)) {
            OSSystemExtensionRequest *request = [
                OSSystemExtensionRequest propertiesRequestForExtension:_dextIdentifier
                    queue:dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0)];
            request.delegate = self;
            [[OSSystemExtensionManager sharedManager] submitRequest:request];
        }
    }
    return self;
}

- (void)activateMyDext {
    [self activateExtension:_dextIdentifier];
}
- (void)removeMyDext {
    [self deactivateExtension:_dextIdentifier];
}

- (NSString *)dextLoadingState {
    switch (self.state) {
        // State descriptions analogous to Swift version
        case VSPSmLoaderStateUnknown:
            return @"VSPSmLoaderStateUnknown";
        case VSPSmLoaderStateUnloaded:
            return @"VSPSmLoaderStateUnloaded";
        case VSPSmLoaderStateActivating:
            return @"VSPSmLoaderStateActivating";
        case VSPSmLoaderStateNeedsApproval:
            return @"VSPSmLoaderStateNeedsApproval";
        case VSPSmLoaderStateActivated:
            return @"VSPSmLoaderStateActivated";
        case VSPSmLoaderStateActivationError:
            return @"VSPSmLoaderStateActivationError";
        case VSPSmLoaderStateRemoval:
            return @"VSPSmLoaderStateRemoval";
    }
}

- (void)activateExtension:(NSString *)dextIdentifier {
    _isUserUnload = NO;

    if (@available(macOS 10.15, *)) {
        OSSystemExtensionRequest *request = [
        OSSystemExtensionRequest activationRequestForExtension:dextIdentifier
                                 queue:dispatch_get_main_queue()];
    request.delegate = self;
        [[OSSystemExtensionManager sharedManager] submitRequest:request];
    } else {
        // Fallback on earlier versions
        return;
    }

    self.state = [VSPSmLoader processState:self.state
                                 withEvent:VSPSmLoaderEventActivationStarted];
}

- (void)deactivateExtension:(NSString *)dextIdentifier {
    _isUserUnload = YES;

    if (@available(macOS 10.15, *)) {
        OSSystemExtensionRequest *request = [
        OSSystemExtensionRequest deactivationRequestForExtension:dextIdentifier
                                 queue:dispatch_get_main_queue()];
    request.delegate = self;
    if (@available(macOS 10.15, *)) {
        [[OSSystemExtensionManager sharedManager] submitRequest:request];
    } else {
        // Fallback on earlier versions
    }
    } else {
        // Fallback on earlier versions
    }

    self.state = [VSPSmLoader processState:self.state
                                 withEvent:VSPSmLoaderEventUninstallStarted];
}

// Implement OSSystemExtensionRequestDelegate methods...

- (OSSystemExtensionReplacementAction)request:(nonnull OSSystemExtensionRequest *)request
                  actionForReplacingExtension:(nonnull OSSystemExtensionProperties *)existing
                                withExtension:(nonnull OSSystemExtensionProperties *)extension
API_AVAILABLE(macos(10.15))
{
    if (@available(macOS 10.15, *)) {
        NSLog(@"Got the upgrade request (%@ -> %@); answering replace.",
              existing.bundleVersion, extension.bundleVersion);
    } else {
        // Fallback on earlier versions
    }
    if (@available(macOS 10.15, *)) {
        os_log(OS_LOG_DEFAULT, "[VSPLM] Got the upgrade request (%@ -> %@); answering replace",
               existing.bundleVersion, extension.bundleVersion);
    } else {
        // Fallback on earlier versions
    }
    if (@available(macOS 10.15, *)) {
        return OSSystemExtensionReplacementActionReplace;
    } else {
        // Fallback on earlier versions
        return 1;
    }
}

- (void)request:(nonnull OSSystemExtensionRequest *)request didFailWithError:(nonnull NSError *)error  API_AVAILABLE(macos(10.15)){
    os_log(OS_LOG_DEFAULT, "[VSPLM] %s", error.description.UTF8String);
    onDidFailWithError(-1, error.description.UTF8String);
}

- (void)request:(nonnull OSSystemExtensionRequest *)request didFinishWithResult:(OSSystemExtensionRequestResult)result  API_AVAILABLE(macos(10.15)){
    if (result == OSSystemExtensionRequestCompleted) {
        os_log(OS_LOG_DEFAULT, "[VSPLM] %s", "Installation successfully.");
    }
    else if (result == OSSystemExtensionRequestWillCompleteAfterReboot) {
        os_log(OS_LOG_DEFAULT, "[VSPLM] %s", "Installation pending. Activate after reboot!");
    } else {
        os_log(OS_LOG_DEFAULT, "[VSPLM] %s  %d", "Installation status", (int)result);
    }
    onDidFinishWithResult((uint32_t)result, "Installation finished");
}

- (void)requestNeedsUserApproval:(nonnull OSSystemExtensionRequest *)request  API_AVAILABLE(macos(10.15)){
    os_log(OS_LOG_DEFAULT, "[VSPLM] %s", "Require user approval.");
    onNeedsUserApproval();
}

@end
