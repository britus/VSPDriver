//
//  Use this file to import your target's public headers that you would like to expose to Swift.
//
#import <stdio.h>
#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#import <IOKit/IOTypes.h>
#import <IOKit/IOKitLib.h>
#import <IOKit/serial/IOSerialKeys.h>
#import <IOKit/serial/ioss.h>

void showMessage(NSString* title, NSString* message);
bool QT_StartApplication();
bool isArchArm64();
bool isQt5();
