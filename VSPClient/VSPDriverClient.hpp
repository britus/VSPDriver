//
//  VSPDriverClient.hpp
//  VSPDriver
//
//  Created by Björn Eschrich on 13.02.25.
//

#ifndef VSPDriverClient_h
#define VSPDriverClient_h

#include <IOKit/IOKitLib.h>

void showMessage(NSString* title, NSString* message);
bool QT_StartApplication();
bool isArchArm64();
bool isQt5();

#endif /* VSPDriverClient_h */
