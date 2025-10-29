// ********************************************************************
// vsploadermodel.m - VSPDriver Dext load/unload (Best Practices Version)
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************

#import <Foundation/Foundation.h>
#import <SystemExtensions/SystemExtensions.h>
#import <SystemConfiguration/SystemConfiguration.h>
#import <objc/runtime.h>
#import <vsploadermodel.h>
#import <vspsmloader.h>

// Callbacks for VSPDriverSetup C++ class
extern void onDidFailWithError(uint64_t code, const char* message);
extern void onDidFinishWithResult(uint64_t code, const char* message);
extern void onNeedsUserApproval(void);

// Error constants
static NSString *const OS_ERROR_STR = @"At least MacOS 10.15 or later required.";
//static NSString *const INVALID_BUNDLE_ID_ERROR = @"Invalid bundle identifier provided";
//static NSString *const NULL_EXTENSION_ERROR = @"System extension properties are null";

// Error domain for custom errors
static NSString *const VSPDLMErrorDomain = @"VSPDLM.ErrorDomain";

@interface VSPLoaderModel ()
@property (nonatomic, strong) NSString *dextBundleId;
@property (nonatomic, assign) BOOL isInstalling;
@property (nonatomic, assign) BOOL isRemoving;
@end

@implementation VSPLoaderModel {
    // Private instance variables
    NSString *_dextBundleId;
    NSError *_lastError;
}

// -----------------------------------------------------------------------
// Constructor - called by VSPDriverSetupWrapper class
// -----------------------------------------------------------------------
- (instancetype)init:(const char*)dextBundleId {
    self = [super init];
    if (self) {
        // Initialize state
        self.state = VSPSmLoaderStateUnknown;
        self.status = 0xf0000000;

        // Validate input parameter
        if (!dextBundleId) {
            [self handleErrorWithCode:0xe1001016
                               message:@"Bundle identifier is null"];
            return nil;
        }

        // Create NSString from C string
        _dextBundleId = [NSString stringWithCString:dextBundleId
                                            encoding:NSUTF8StringEncoding];

        // Validate bundle ID
        if (!_dextBundleId || _dextBundleId.length == 0) {
            [self handleErrorWithCode:0xe1001017
                               message:@"Invalid bundle identifier"];
            return nil;
        }

        NSLog(@"[VSPDLM] Using bundle Id: %@", _dextBundleId);

        // System Extension Properties Check
        if (@available(macOS 12, *)) {
            [self fetchExtensionProperties];
        } else {
            [self handleErrorWithCode:0xe1001015
                               message:OS_ERROR_STR];
        }
    }
    return self;
}

// -----------------------------------------------------------------------
// Fetch extension properties for validation
// -----------------------------------------------------------------------
- (void)fetchExtensionProperties {
    if (!_dextBundleId) {
        [self handleErrorWithCode:0xe1001017
                           message:@"Bundle identifier not set"];
        return;
    }

    // Use the system extension manager to query properties
    OSSystemExtensionRequest *request = [
        OSSystemExtensionRequest propertiesRequestForExtension:_dextBundleId
                queue:dispatch_get_global_queue(QOS_CLASS_BACKGROUND, 0)];
    request.delegate = self;

    // Submit the request to get extension properties
    [[OSSystemExtensionManager sharedManager] submitRequest:request];
}

// -----------------------------------------------------------------------
// Install embedded Dext - called by VSPDriverSetupWrapper class
// -----------------------------------------------------------------------
- (void)activateMyDext {
    [self activateExtension:_dextBundleId];
}

// -----------------------------------------------------------------------
// Remove existing Dext - called by VSPDriverSetupWrapper class
// -----------------------------------------------------------------------
- (void)removeMyDext {
    [self deactivateExtension:_dextBundleId];
}

// -----------------------------------------------------------------------
// Install embedded Dext
// -----------------------------------------------------------------------
- (void)activateExtension:(NSString *)dextBundleId {
    // Validate input
    if (!_dextBundleId) {
        [self handleErrorWithCode:0xe1001017
                           message:@"Bundle identifier not set"];
        return;
    }

    // Prevent concurrent operations
    if (self.isInstalling || self.isRemoving) {
        [self handleErrorWithCode:0xe1001018
                           message:@"Operation already in progress"];
        return;
    }

    self.isInstalling = YES;
    self.status = 0xf1000000;

    if (@available(macOS 10.15, *)) {
        OSSystemExtensionRequest *request;
        request = [OSSystemExtensionRequest activationRequestForExtension:dextBundleId
                                                                    queue:dispatch_get_main_queue()];
        request.delegate = self;

        // Update state
        self.state = [VSPSmLoader processState:self.state withEvent:VSPSmLoaderEventActivationStarted];

        NSLog(@"[VSPDLM] Starting activation of extension: %@", dextBundleId);

        [[OSSystemExtensionManager sharedManager] submitRequest:request];
    } else {
        [self handleErrorWithCode:0xe1001015
                           message:OS_ERROR_STR];
    }
}

// -----------------------------------------------------------------------
// Remove existing Dext
// -----------------------------------------------------------------------
- (void)deactivateExtension:(NSString *)dextBundleId {
    // Validate input
    if (!_dextBundleId) {
        [self handleErrorWithCode:0xe1001017
                           message:@"Bundle identifier not set"];
        return;
    }

    // Prevent concurrent operations
    if (self.isInstalling || self.isRemoving) {
        [self handleErrorWithCode:0xe1001018
                           message:@"Operation already in progress"];
        return;
    }

    self.isRemoving = YES;
    self.status = 0xf2000000;

    if (@available(macOS 10.15, *)) {
        OSSystemExtensionRequest *request;
        request = [OSSystemExtensionRequest deactivationRequestForExtension:dextBundleId
                                                                      queue:dispatch_get_main_queue()];
        request.delegate = self;

        // Update state
        self.state = [VSPSmLoader processState:self.state withEvent:VSPSmLoaderEventUninstallStarted];

        NSLog(@"[VSPDLM] Starting deactivation of extension: %@", dextBundleId);

        [[OSSystemExtensionManager sharedManager] submitRequest:request];
    } else {
        [self handleErrorWithCode:0xe1001015
                           message:OS_ERROR_STR];
    }
}

// -----------------------------------------------------------
// Implement OSSystemExtensionRequestDelegate methods...
// -----------------------------------------------------------

// -----------------------------------------------------------------------
// Handle replacement actions for system extensions
// -----------------------------------------------------------------------
- (OSSystemExtensionReplacementAction)request:(nonnull OSSystemExtensionRequest *)request
                  actionForReplacingExtension:(nonnull OSSystemExtensionProperties *)existing
                                withExtension:(nonnull OSSystemExtensionProperties *)extension API_AVAILABLE(macos(10.15)) {

    if (!existing || !extension) {
        [self handleErrorWithCode:0xe1001019
                           message:@"Extension properties are null"];
        return OSSystemExtensionReplacementActionCancel;
    }

    NSComparisonResult result = [existing.bundleVersion compare:extension.bundleVersion];
    NSLog(@"[VSPDLM] Got the upgrade request (%@ -> %@) ordering=%ld; answering replace.",
           existing.bundleVersion, extension.bundleVersion, result);

    // Always replace - this is the typical behavior for driver updates
    return OSSystemExtensionReplacementActionReplace;
}

// -----------------------------------------------------------------------
// Handle errors from the system extension manager
// -----------------------------------------------------------------------
- (void)request:(nonnull OSSystemExtensionRequest *)request didFailWithError:(nonnull NSError *)error API_AVAILABLE(macos(10.15)) {
    // Log detailed error information
    NSLog(@"[VSPDLM] Error 0x%lx %@\n", error.code, error.description);

    // Log additional error details if available
    if (error.userInfo[NSLocalizedFailureReasonErrorKey]) {
        NSLog(@"[VSPDLM] Failure reason: %@", error.userInfo[NSLocalizedFailureReasonErrorKey]);
    }

    if (error.userInfo[NSLocalizedRecoverySuggestionErrorKey]) {
        NSLog(@"[VSPDLM] Recovery suggestion: %@", error.userInfo[NSLocalizedRecoverySuggestionErrorKey]);
    }

    // Store last error for potential debugging
    _lastError = [error copy];

    // Handle specific error conditions
    [self handleSystemExtensionError:error];
}

// -----------------------------------------------------------------------
// Handle specific system extension errors
// -----------------------------------------------------------------------
- (void)handleSystemExtensionError:(NSError *)error {
    // TODO Check for specific error codes and provide better user feedback
    switch (error.code) {
        case 0x00: //
            [self handleErrorWithCode:error.code
                               message:[NSString stringWithFormat:@"System extension error: %@", error.description]];
            break;
        default:
            [self handleErrorWithCode:error.code
                               message:[NSString stringWithFormat:@"System extension error: %@", error.description]];
            break;
    }
}

// -----------------------------------------------------------------------
// Handle successful completion of system extension operations
// -----------------------------------------------------------------------
- (void)request:(nonnull OSSystemExtensionRequest *)request didFinishWithResult:(OSSystemExtensionRequestResult)result API_AVAILABLE(macos(10.15)) {
    self.status |= result;

    NSLog(@"[VSPDLM] System extension request completed with status: %d", (int)result);

    // Reset operation flags
    self.isInstalling = NO;
    self.isRemoving = NO;

    switch (result) {
        case OSSystemExtensionRequestCompleted:
            NSLog(@"[VSPDLM] OS system extension request completed successfully.");

            if (self.state == VSPSmLoaderStateRemoval) {
                [self handleSuccessWithMessage:@"Driver successfully removed."];
            } else {
                [self handleSuccessWithMessage:@"Driver successfully activated."];
            }
            break;

        case OSSystemExtensionRequestWillCompleteAfterReboot:
            NSLog(@"[VSPDLM] OS system extension request will complete after reboot.");

            if (self.state == VSPSmLoaderStateRemoval) {
                [self handleSuccessWithMessage:@"Will remove VSP driver after reboot."];
            } else {
                [self handleSuccessWithMessage:@"Activate VSP driver after reboot."];
            }
            break;

        default:
            NSLog(@"[VSPDLM] System Extension request status: %d", (int)result);

            if (self.state == VSPSmLoaderStateRemoval) {
                [self handleErrorWithCode:self.status
                                   message:@"Remove VSP driver with status."];
            } else {
                [self handleErrorWithCode:self.status
                                   message:@"Activate VSP driver with status."];
            }
            break;
    }
}

// -----------------------------------------------------------------------
// Handle user approval requirement
// -----------------------------------------------------------------------
- (void)requestNeedsUserApproval:(nonnull OSSystemExtensionRequest *)request API_AVAILABLE(macos(10.15)) {
    NSLog(@"[VSPDLM] Require user approval for system extension operation.");

    // Reset flags to prevent multiple notifications
    self.isInstalling = NO;
    self.isRemoving = NO;

    // Notify C++ callback that user approval is needed
    onNeedsUserApproval();
}

// -----------------------------------------------------------------------
// Handle discovered system extension properties
// -----------------------------------------------------------------------
- (void)request:(nonnull OSSystemExtensionRequest *)request
            foundProperties:(NSArray<OSSystemExtensionProperties *> *) properties API_AVAILABLE(macos(10.15)) {
    NSLog(@"[VSPDLM] Found properties count: %ld", (unsigned long)properties.count);

    if (!properties || properties.count == 0) {
        NSLog(@"[VSPDLM] No system extension properties found.");
        return;
    }

    for (OSSystemExtensionProperties *obj in properties) {
        if (!obj) {
            NSLog(@"[VSPDLM] Null system extension properties object found.");
            continue;
        }

        // Use modern property access instead of runtime introspection
        NSLog(@"[VSPDLM] System extension properties object: %@", obj);

        // Log key properties using direct access (safer than runtime introspection)
        if (obj.bundleIdentifier) {
            NSLog(@"[VSPDLM] Bundle Identifier: %@", obj.bundleIdentifier);
        }

        if (obj.bundleVersion) {
            NSLog(@"[VSPDLM] Bundle Version: %@", obj.bundleVersion);
        }

        if (obj.isEnabled) {
            NSLog(@"[VSPDLM] Is enabled: %d", obj.isEnabled);
        }
        if (obj.isAwaitingUserApproval) {
            NSLog(@"[VSPDLM] Is awaiting user approval: %d", obj.isAwaitingUserApproval);
        }
        if (obj.isProxy) {
            NSLog(@"[VSPDLM] Is proxy: %d", obj.isProxy);
        }
        if (obj.isUninstalling) {
            NSLog(@"[VSPDLM] Is proxy: %d", obj.isUninstalling);
        }

        unsigned int propCount;
        objc_property_t *propertyList = class_copyPropertyList([obj class], &propCount);
        if (propertyList == NULL)
            continue;

        NSLog(@"[VSPDLM] -------------------------------");
        for (unsigned int i = 0; i < propCount; i++) {
            if (propertyList[i] == NULL)
                continue;
            const char *propName = property_getName(propertyList[i]);
            if (propName) {
                NSString *key = [NSString stringWithUTF8String:propName];
                if (key != NULL) {
                    id value = [obj valueForKey:key];
                    // onPropertyInfo(key, value)
                    NSLog(@"[VSPDLM] %@ = %@", key, value);
                }
            }
        }
    }
}

// -----------------------------------------------------------------------
// Helper method to handle success cases
// -----------------------------------------------------------------------
- (void)handleSuccessWithMessage:(NSString *)message {
    // Clear any previous error state
    _lastError = nil;

    // Call C++ callback with success information
    onDidFinishWithResult(self.status, message.UTF8String);
}

// -----------------------------------------------------------------------
// Helper method to handle error cases
// -----------------------------------------------------------------------
- (void)handleErrorWithCode:(uint64_t)errorCode message:(NSString *)message {
    NSLog(@"[VSPDLM] Error - Code: 0x%llx, Message: %@", errorCode, message);

    // Store error for debugging purposes
    NSError *error = [NSError errorWithDomain:VSPDLMErrorDomain
                                         code:(NSInteger)errorCode
                                     userInfo:@{NSLocalizedDescriptionKey: message}];
    _lastError = error;

    // Call C++ callback with error information
    onDidFailWithError(errorCode, message.UTF8String);
}

// -----------------------------------------------------------------------
// Helper method to get last error for debugging
// -----------------------------------------------------------------------
- (NSError *)lastError {
    return _lastError;
}

@end
