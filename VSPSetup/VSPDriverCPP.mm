// ********************************************************************
// VSPDriverCPP.mm - VSPDriver user setup/install
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
#include <stdio.h>
#import <Foundation/Foundation.h>
#import <SystemExtensions/SystemExtensions.h>
#import <VSPLoaderModel.h>
#import <VSPDriverSetup.hpp>

@interface VSPDriverSetupWrapper : NSObject

@property (nonatomic, strong) VSPLoaderModel *loaderModel;

- (void) activate;
- (void) deactivate;
- (NSString *) stateDescription;

@end

static VSPDriverSetup* g_callback;
static uint32_t g_error_code;
static uint8_t g_cb_index;
static char g_message[256];

@implementation VSPDriverSetupWrapper

- (instancetype)init {
    self = [super init];
    if (self) {
        _loaderModel = [[VSPLoaderModel alloc]init];
    }
    return self;
}

- (void) activate {
    [_loaderModel activateMyDext];
}

- (void) deactivate
{
    [_loaderModel removeMyDext];
}

- (NSString *) stateDescription
{
    return [_loaderModel dextLoadingState];
}

@end

// ---------------------------------------------------
// C callbacks for VSPLoaderModel.m
// ---------------------------------------------------

extern "C" {
void onDidFailWithError(uint32_t code, const char* message)
{
    strncpy(g_message, message, sizeof(g_message)-1);
    g_error_code = code;
    g_cb_index = 1;
    g_callback->OnDidFailWithError(g_error_code, g_message);
}

void onDidFinishWithResult(uint32_t code, const char* message)
{
    strncpy(g_message, message, sizeof(g_message)-1);
    g_error_code = code;
    g_cb_index = 2;
    g_callback->OnDidFinishWithResult(g_error_code, g_message);
}

void onNeedsUserApproval()
{
    g_message[0] = 0;
    g_error_code = 0;
    g_cb_index = 3;
    g_callback->OnNeedsUserApproval();
}
} /* "C" */

// ---------------------------------------------------
// C++ class for external use (exported)
// ---------------------------------------------------

VSPDriverSetup::VSPDriverSetup() :
    _loader((__bridge void*)[[VSPDriverSetupWrapper alloc] init])
{
    g_callback = this;
}

void VSPDriverSetup::activateDriver()
{
    [(__bridge VSPDriverSetupWrapper*)_loader activate];
#if 0
    switch(g_cb_index) {
        case 1: {OnDidFailWithError(g_error_code, g_message); break;}
        case 2: {OnDidFinishWithResult(g_error_code, g_message); break;}
        case 3: {OnNeedsUserApproval(); break;}
    }
#endif
}

void VSPDriverSetup::deactivateDriver()
{
    [(__bridge VSPDriverSetupWrapper*)_loader deactivate];
#if 0
    switch(g_cb_index) {
        case 1: {OnDidFailWithError(g_error_code, g_message); break;}
        case 2: {OnDidFinishWithResult(g_error_code, g_message); break;}
        case 3: {OnNeedsUserApproval(); break;}
    }
#endif
}

std::string VSPDriverSetup::getDriverState() const
{
    return [(__bridge VSPDriverSetupWrapper*)_loader stateDescription].UTF8String;
}

void VSPDriverSetup::OnDidFailWithError(uint32_t /*code*/, const char* /*message*/)
{
    // nothing, override this method
}

void VSPDriverSetup::OnDidFinishWithResult(uint32_t /*code*/, const char* /*message*/)
{
    // nothing, override this method
}

void VSPDriverSetup::OnNeedsUserApproval()
{
    // nothing, override this method
}
