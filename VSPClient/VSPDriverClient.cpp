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

bool isArchArm64() {
#if defined(__arm64)
    return true;
#else
    return false;
#endif
}

bool isQt5() {
#if defined(VSP_x86_64)
    return true;
#else
    return false;
#endif
}

// MARK: Wrap into QT app
bool QT_StartApplication() {
#if defined(__arm64) || defined(VSP_x86_64)
    char* argv[] = {};
    return qt_main(0, argv);
#else
    return KERN_INVALID_CAPABILITY;
#endif
}

} // extern "C"
