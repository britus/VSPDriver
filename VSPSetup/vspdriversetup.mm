// ********************************************************************
// VSPDriverCPP.mm - VSPDriver user setup/install
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
#import <stdio.h>
#import <Foundation/Foundation.h>
#import <SystemExtensions/SystemExtensions.h>
#import <vsploadermodel.h>
#import <vspdriversetup.hpp>

@interface VSPDriverSetupWrapper : NSObject

@property (nonatomic, strong) VSPLoaderModel *loaderModel;

- (void) activate;
- (void) deactivate;

@end

static VSPDriverSetup* g_callback;
static char g_message[256];

@implementation VSPDriverSetupWrapper

- (instancetype)init:(const char*)dextBundleId
{
    self = [super init];
    if (self) {
        fprintf(stderr, "[VSPDS] Setup using bundle Id: %s\n", dextBundleId);
        _loaderModel = [[VSPLoaderModel alloc]init:dextBundleId];
    }
    return self;
}

-(void)dealloc
{
    [_loaderModel release];
    [super dealloc];
}

- (void) activate
{
    [_loaderModel activateMyDext];
}

- (void) deactivate
{
    [_loaderModel removeMyDext];
}

@end

// ---------------------------------------------------
// C callbacks for VSPLoaderModel.m
// ---------------------------------------------------

extern "C" {
void onDidFailWithError(uint64_t code, const char* message)
{
    snprintf(g_message, sizeof(g_message) - 1, "\n%s", message);
    g_callback->OnDidFailWithError(code, g_message);
}

void onDidFinishWithResult(uint64_t code, const char* message)
{
    strncpy(g_message, message, sizeof(g_message)-1);
    g_callback->OnDidFinishWithResult(code, g_message);
}

void onNeedsUserApproval()
{
    g_message[0] = 0;
    g_callback->OnNeedsUserApproval();
}
} /* "C" */

// ---------------------------------------------------
// C++ class for external use (exported)
// ---------------------------------------------------

VSPDriverSetup::VSPDriverSetup(const char* dextBundleId) :
    _loader((__bridge void*)[[VSPDriverSetupWrapper alloc] init:dextBundleId])
{
    g_callback = this;
}

void VSPDriverSetup::activateDriver()
{
    [(__bridge VSPDriverSetupWrapper*)_loader activate];
}

void VSPDriverSetup::deactivateDriver()
{
    [(__bridge VSPDriverSetupWrapper*)_loader deactivate];
}

void VSPDriverSetup::OnDidFailWithError(uint64_t /*code*/, const char* /*message*/)
{
    // nothing, override this method
}

void VSPDriverSetup::OnDidFinishWithResult(uint64_t /*code*/, const char* /*message*/)
{
    // nothing, override this method
}

void VSPDriverSetup::OnNeedsUserApproval()
{
    // nothing, override this method
}
