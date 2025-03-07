//
//  VSPInfoDialog.m
//  VSPClient
//
//  Created by Björn Eschrich on 06.03.25.
//

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

@interface AlertHelper : NSObject

// Statische Methode zum Anzeigen eines Alerts
+ (void)showAlertWithTitle:(NSString *)title message:(NSString *)message;

@end

@implementation AlertHelper

+ (void)showAlertWithTitle:(NSString *)title message:(NSString *)message {
    // Erstelle das Alert-Fenster
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:title];  // Titel setzen
    [alert setInformativeText:message];  // Nachricht setzen
    [alert addButtonWithTitle:@"OK"];  // OK-Button hinzufügen
    
    // Zeige das Alert an
    [alert runModal];
}

@end

void showMessage(NSString* title, NSString* message) {
    @autoreleasepool {
        [AlertHelper showAlertWithTitle:title message:message];
    }
}
