// ********************************************************************
// VSPInfoDialog.m - Show a message on startup
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

@interface AlertHelper : NSObject
+ (void)showAlertWithTitle:(NSString *)title message:(NSString *)message;
@end

@implementation AlertHelper
+ (void)showAlertWithTitle:(NSString *)title message:(NSString *)message {
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:title];
    [alert setInformativeText:message];
    [alert addButtonWithTitle:@"OK"];
    [alert runModal];
}
@end

void showMessage(NSString* title, NSString* message) {
    @autoreleasepool {
        [AlertHelper showAlertWithTitle:title message:message];
    }
}
