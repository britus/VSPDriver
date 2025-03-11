// ********************************************************************
// vsploadmodel.m - VSPDriver Dext load/unload
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

// callbacks for VSPDriverSetup C++ class
extern void onDidFailWithError(uint64_t code, const char* message);
extern void onDidFinishWithResult(uint64_t code, const char* message);
extern void onNeedsUserApproval(void);

#define OS_ERROR_STR "At least MacOS 10.15 or later required."

@implementation VSPLoaderModel {
    NSString* m_dextBundleId;
}

// -----------------------------------------------------------------------
// Constructor - called by VSPDriverSetupWrapper class
- (instancetype)init:(const char*)dextBundleId
{
    self = [super init];
    if (self)
    {
        self.state     = VSPSmLoaderStateUnknown;
        self.status    = 0xf0000000;
        m_dextBundleId = [NSString stringWithCString:dextBundleId encoding:NSUTF8StringEncoding];

        if (m_dextBundleId != NULL)
        {
            NSLog(@"[VSPDLM] Using bundle Id: %@\n", m_dextBundleId);

            // System Extension Properties Check
            if (@available(macOS 12, *)) {
                OSSystemExtensionRequest *request = [
                    OSSystemExtensionRequest propertiesRequestForExtension:m_dextBundleId
                        queue:dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0)];
                request.delegate = self;

                [[OSSystemExtensionManager sharedManager] submitRequest:request];
            }
            else {
                onDidFailWithError(0xe1001015, OS_ERROR_STR);
            }
        }
    }
    return self;
}

// -----------------------------------------------------------------------
// Short: Install embedded Dext - called by VSPDriverSetupWrapper class
- (void)activateMyDext
{
    [self activateExtension:m_dextBundleId];
}

// -----------------------------------------------------------------------
// Short: Remove existing Dext - called by VSPDriverSetupWrapper class
- (void)removeMyDext
{
    [self deactivateExtension:m_dextBundleId];
}

// -----------------------------------------------------------------------
// Install embedded Dext
- (void)activateExtension:(NSString *)dextBundleId
{
    self.status = 0xf1000000;

    if (@available(macOS 10.15, *)) {
        OSSystemExtensionRequest *request;
        request = [OSSystemExtensionRequest activationRequestForExtension:dextBundleId queue:dispatch_get_main_queue()];
        request.delegate = self;
        self.state = [VSPSmLoader processState:self.state withEvent:VSPSmLoaderEventActivationStarted];

        [[OSSystemExtensionManager sharedManager] submitRequest:request];
    }
    else {
        onDidFailWithError(0xe1001015, OS_ERROR_STR);
        return;
    }
}

// -----------------------------------------------------------------------
// Remove existing Dext
- (void)deactivateExtension:(NSString *)dextBundleId
{
    self.status = 0xf2000000;

    if (@available(macOS 10.15, *)) {
        OSSystemExtensionRequest *request;
        request = [OSSystemExtensionRequest deactivationRequestForExtension:dextBundleId queue:dispatch_get_main_queue()];
        request.delegate = self;
        self.state = [VSPSmLoader processState:self.state withEvent:VSPSmLoaderEventUninstallStarted];

        [[OSSystemExtensionManager sharedManager] submitRequest:request];
    }
    else {
        onDidFailWithError(0xe1001015, OS_ERROR_STR);
        return;
    }
}

// -----------------------------------------------------------
// Implement OSSystemExtensionRequestDelegate methods...
// -----------------------------------------------------------

// -----------------------------------------------------------------------
// Event raised to inform the OS what we want to do in the Dext
// installation process
- (OSSystemExtensionReplacementAction)request:(nonnull OSSystemExtensionRequest *)request
                  actionForReplacingExtension:(nonnull OSSystemExtensionProperties *)existing
                                withExtension:(nonnull OSSystemExtensionProperties *)extension API_AVAILABLE(macos(10.15))
{

    NSLog(@"[VSPDLM] Got the upgrade request (%@ -> %@); answering replace.\n",
           existing.bundleVersion, extension.bundleVersion);

     NSComparisonResult result = [existing.bundleVersion compare:extension.bundleVersion];

     // up- or downgrade
     switch (result) {
        case NSOrderedAscending:
            return OSSystemExtensionReplacementActionReplace;
        case NSOrderedDescending:
            return OSSystemExtensionReplacementActionReplace;
        case NSOrderedSame:
            return OSSystemExtensionReplacementActionReplace;
    }

    return OSSystemExtensionReplacementActionCancel;
}

// -----------------------------------------------------------------------
// Event raised if OS tell us something is wrong. Mostly security or
// entitlements problems :)
- (void)request:(nonnull OSSystemExtensionRequest *)request didFailWithError:(nonnull NSError *)error API_AVAILABLE(macos(10.15))
{
    NSLog(@"[VSPDLM] Error 0x%lx %@\n", error.code, error.description);

    onDidFailWithError(error.code, error.description.UTF8String);
}

// -----------------------------------------------------------------------
// Event raised if OS has been installed/removed the Dext
- (void)request:(nonnull OSSystemExtensionRequest *)request didFinishWithResult:(OSSystemExtensionRequestResult)result API_AVAILABLE(macos(10.15))
{
    self.status |= result;

    if (result == OSSystemExtensionRequestCompleted)
    {
        NSLog(@"[VSPDLM] OS system extension request completed.\n");

        if (result == 0) {
            if (self.state == VSPSmLoaderStateRemoval) {
                onDidFinishWithResult(self.status, "Driver successfully removed.");
            }
            else {
                onDidFinishWithResult(self.status, "Driver successfully activated.");
            }
        }
        else {
            onDidFinishWithResult(self.status, "Wait for VSP driver activation.");
        }
    }
    else if (result == OSSystemExtensionRequestWillCompleteAfterReboot)
    {
        NSLog(@"[VSPDLM] OS system extension request will complete after reboot.\n");

        if (self.state == VSPSmLoaderStateRemoval) {
            NSLog(@"[VSPDLM] Removal pending. Removed after reboot.\n");
            onDidFinishWithResult(self.status, "Will remove VSP driver after reboot.");
        }
        else {
            NSLog(@"[VSPDLM] Installation pending. Activate after reboot.\n");
            onDidFinishWithResult(self.status, "Activate VSP driver after reboot.");
        }
    }
    else {
        NSLog(@"[VSPDLM] System Extension request status: %d\n", (int)result);

        if (self.state == VSPSmLoaderStateRemoval) {
            onDidFinishWithResult(self.status, "Remove VSP driver with status.");
        }
        else {
            onDidFinishWithResult(self.status, "Activate VSP driver with status.");
        }
    }
}

// -----------------------------------------------------------------------
// Event raised if user approval is required to install/remove the Dext
- (void)requestNeedsUserApproval:(nonnull OSSystemExtensionRequest *)request API_AVAILABLE(macos(10.15))
{
    NSLog(@"[VSPDLM] Require user approval.\n");

    onNeedsUserApproval();
}

// -----------------------------------------------------------------------
// Shows installed/known versions of the VSP Dext in the OS
- (void)request:(nonnull OSSystemExtensionRequest *)request foundProperties:(NSArray<OSSystemExtensionProperties *> *) properties API_AVAILABLE(macos(10.15))
{
    NSLog(@"[VSPDLM] properties count: %ld\n", (unsigned long)properties.count);

    for (OSSystemExtensionProperties *obj in properties) {
        unsigned int count;

        objc_property_t *propertyList = class_copyPropertyList([obj class], &count);
        if (propertyList == NULL)
            continue;

        NSLog(@"[VSPDLM] Object: %@", obj);

        for (unsigned int i = 0; i < count; i++) {
            if (propertyList[i] == NULL)
                continue;

            const char *propName = property_getName(propertyList[i]);
            if (propName) {
                NSString *key = [NSString stringWithUTF8String:propName];
                if (key != NULL) {
                    id value = [obj valueForKey:key];

                    NSLog(@"[VSPDLM] %@ = %@", key, value);
                }
            }
        }

        free(propertyList);
    }
}


@end
