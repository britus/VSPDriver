// ********************************************************************
// VSPDriverSetup.cpp - Interface to install VSPDriver DEXT
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
#pragma once
#include <string>
#include <vspsetup_global.h>

class VSPSETUP_EXPORT VSPDriverSetup
{
public:
    VSPDriverSetup();
    void activateDriver();
    void deactivateDriver();
    std::string getDriverState() const;

    virtual void OnDidFailWithError(uint32_t /*code*/, const char* /*message*/);
    virtual void OnDidFinishWithResult(uint32_t /*code*/, const char* /*message*/);
    virtual void OnNeedsUserApproval();

private:
    void* _loader; // Opaque pointer to Objective-C object
};
