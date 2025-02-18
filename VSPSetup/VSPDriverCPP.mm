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
static char g_message[256];

@implementation VSPDriverSetupWrapper

- (instancetype)init {
    self = [super init];
    if (self) {
        _loaderModel = [[VSPLoaderModel alloc]init];
    }
    return self;
}
/* ARC ??
-(void)dealloc {
    [_loaderModel release];
    [super dealloc];
}
*/
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
    g_callback->OnDidFailWithError(code, g_message);
}

void onDidFinishWithResult(uint32_t code, const char* message)
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

VSPDriverSetup::VSPDriverSetup() :
    _loader((__bridge void*)[[VSPDriverSetupWrapper alloc] init])
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
