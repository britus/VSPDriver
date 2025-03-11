// ********************************************************************
// vspdriversetup.cpp - Interface to install/uninstall VSPDriver Dext
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
#pragma once
#include "vspsetup_global.h"
#include <inttypes.h>

class VSPSETUP_EXPORT VSPDriverSetup
{
public:
    VSPDriverSetup(const char* dextBundleId);
    void activateDriver();
    void deactivateDriver();

    // override this methods to get events
    virtual void OnDidFailWithError(uint64_t /*code*/, const char* /*message*/);
    virtual void OnDidFinishWithResult(uint64_t /*code*/, const char* /*message*/);
    virtual void OnNeedsUserApproval();

private:
    void* _loader; // Opaque pointer to Objective-C object
};
