// ********************************************************************
// VSPController.cpp - VSP controller client
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
#include <stdio.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>

extern "C" {
    
extern bool qt_main(int argc, char** argv);

// MARK: Wrap into QT app
bool QT_StartApplication() {
    char* argv[] = {};
    return qt_main(0, argv);
}

} // extern "C"
