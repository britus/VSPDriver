// ********************************************************************
// VSPController.cpp - VSP client entry (Swift -> VSPClient framework)
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************

#include <stdio.h>
//#include <CoreFoundation/CoreFoundation.h>
//#include <IOKit/IOTypes.h>
//#include <IOKit/IOKitLib.h>
//#include <IOKit/serial/IOSerialKeys.h>
//#include <IOKit/serial/ioss.h>

extern "C" {

// exported by VSPClient.framework
extern int qt_main(int argc, char* argv[]);

// Open QT main window
bool QT_MainEntry()
{
    char* argv[] = {};
    return qt_main(0, argv) == 0;
}

} // extern "C"
