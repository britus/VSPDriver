// ********************************************************************
// VSPUserClient.cpp - VSP controller object
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************

// -- OS
#include <os/log.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#include <DriverKit/IOLib.h>
#include <DriverKit/IOTypes.h>
#include <DriverKit/IOService.h>
#include <DriverKit/IOUserClient.h>
#include <DriverKit/OSDictionary.h>
#include <DriverKit/OSData.h>
#include <DriverKit/OSAction.h>
#include <DriverKit/IODataQueueDispatchSource.h>
#include <DriverKit/IOInterruptDispatchSource.h>
#include <DriverKit/IOTimerDispatchSource.h>

// -- My
#include "VSPUserClient.h"
#include "VSPController.h"
#include "VSPLogger.h"
#include "VSPDriver.h"
using namespace VSPController;

#define LOG_PREFIX "VSPUserClient"

struct VSPUserClient_IVars {
    IOService* m_provider = nullptr;
    VSPDriver* m_parent = nullptr;
    IODispatchQueue* m_eventQueue = nullptr;
    IOTimerDispatchSource* m_eventSource = nullptr;
    OSAction* m_eventAction = nullptr;
    OSAction* m_cbAction = nullptr;
};

#define VSP_METHOD(x) ((IOUserClientMethodFunction) &VSPUserClient::x)

// Define all possible commands with its parameters and
// callback entry points.
const IOUserClientMethodDispatch uc_methods[vspLastCommand] = {
    // Register async callback
    [vspControlSetCallback] =
    {
        .function = VSP_METHOD(exSetCallback),
        .checkCompletionExists = true,
        .checkScalarInputCount = 0,
        .checkStructureInputSize = sizeof(TVSPControllerData),
        .checkScalarOutputCount = 0,
        .checkStructureOutputSize = sizeof(TVSPControllerData),
    },
    // Async request to link two ports together
    [vspControlGetPortList] =
    {
        .function = VSP_METHOD(exLinkPorts),
        // Don't care about completion
        .checkCompletionExists = -1U,
        .checkScalarInputCount = 0,
        .checkStructureInputSize = sizeof(TVSPControllerData),
        .checkScalarOutputCount = 0,
        .checkStructureOutputSize = 0,
    },
    // Async request to link two ports together
    [vspControlLinkPorts] =
    {
        .function = VSP_METHOD(exLinkPorts),
        // Don't care about completion
        .checkCompletionExists = -1U,
        .checkScalarInputCount = 0,
        .checkStructureInputSize = sizeof(TVSPControllerData),
        .checkScalarOutputCount = 0,
        .checkStructureOutputSize = 0,
    },
    // Async request to link two ports together
    [vspControlUnlinkPorts] =
    {
        .function = VSP_METHOD(exLinkPorts),
        // Don't care about completion
        .checkCompletionExists = -1U,
        .checkScalarInputCount = 0,
        .checkStructureInputSize = sizeof(TVSPControllerData),
        .checkScalarOutputCount = 0,
        .checkStructureOutputSize = 0,
    },
};

// --------------------------------------------------------------------
// Allocate internal resources
//
bool VSPUserClient::init()
{
    bool result;
    
    VSPLog(LOG_PREFIX, "init called.\n");
    
    if (!(result = super::init())) {
        VSPLog(LOG_PREFIX, "free (super) falsed. result=%d\n", result);
        goto finish;
    }

    // Create instance state resource
    ivars = IONewZero(VSPUserClient_IVars, 1);
    if (!ivars) {
        VSPLog(LOG_PREFIX, "Unable to allocate driver data.\n");
        result = false;
        goto finish;
    }
    
    return true;
    
finish:
    return result;
}

// --------------------------------------------------------------------
// Release internal resources
//
void VSPUserClient::free()
{
    VSPLog(LOG_PREFIX, "free called.\n");
    
    // Release instance state resource
    IOSafeDeleteNULL(ivars, VSPUserClient_IVars, 1);
    super::free();
}

// --------------------------------------------------------------------
// Start_Impl(IOService* provider)
//
kern_return_t IMPL(VSPUserClient, Start)
{
    kern_return_t ret;
    
    VSPLog(LOG_PREFIX, "Start: called.\n");
   
    // sane check our driver instance vars
    if (!ivars) {
        VSPLog(LOG_PREFIX, "Start: Private driver instance is NULL\n");
        return kIOReturnInvalid;
    }

    // Start service instance (Apple style super call)
    ret = Start(provider, SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "Start(super): failed. code=%d\n", ret);
        return ret;
    }
    
    ret = IODispatchQueue::Create("NullDriverUserClientDispatchQueue", 0, 0, &ivars->m_eventQueue);
    if (ret != kIOReturnSuccess || !ivars->m_eventQueue)
    {
        VSPLog(LOG_PREFIX, "Start() - Failed to create dispatch queue with error: 0x%08x.", ret);
        if (ret == kIOReturnSuccess) {
            ret = kIOReturnInvalid;
        }
        return ret;
    }

    ret = IOTimerDispatchSource::Create(ivars->m_eventQueue, &ivars->m_eventSource);
    if (ret != kIOReturnSuccess || !ivars->m_eventSource)
    {
        VSPLog(LOG_PREFIX, "Start() - Failed to create dispatch source with error: 0x%08x.", ret);
        if (ret == kIOReturnSuccess) {
            ret = kIOReturnInvalid;
        }
        return ret;
    }

    ret = CreateActionAsyncCallback(sizeof(TVSPControllerData), &ivars->m_eventAction);
    if (ret != kIOReturnSuccess || !ivars->m_eventAction)
    {
        VSPLog(LOG_PREFIX, "Start() - Failed to create action for simulated async event with error: 0x%08x.", ret);
        if (ret == kIOReturnSuccess) {
            ret = kIOReturnInvalid;
        }
        return ret;
    }

    ret = ivars->m_eventSource->SetHandler(ivars->m_eventAction);
    if (ret != kIOReturnSuccess)
    {
        VSPLog(LOG_PREFIX, "Start() - Failed to assign simulated action to handler with error: 0x%08x.", ret);
        return ret;
    }

    ret = RegisterService();
    if (ret != kIOReturnSuccess)
    {
        VSPLog(LOG_PREFIX, "Start() - Failed to register service with error: 0x%08x.", ret);
        return ret;
    }

    VSPLog(LOG_PREFIX, "User client successfully started.\n");
    return ret;
}

// --------------------------------------------------------------------
// Stop_Impl(IOService* provider)
//
kern_return_t IMPL(VSPUserClient, Stop)
{
    kern_return_t ret;
    
    VSPLog(LOG_PREFIX, "Stop called.\n");
    
    OSSafeReleaseNULL(ivars->m_eventAction);
    OSSafeReleaseNULL(ivars->m_eventSource);
    OSSafeReleaseNULL(ivars->m_eventQueue);
    OSSafeReleaseNULL(ivars->m_cbAction);

    // service instance (Apple style super call)
    if ((ret= Stop(provider, SUPERDISPATCH)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "Stop (suprt) failed. code=%d\n", ret);
    } else {
        VSPLog(LOG_PREFIX, "User client successfully removed.\n");
    }
    
    return ret;
}

// --------------------------------------------------------------------
// AsyncCallback_Impl(OSAction* action, uint64_t time)
//
void IMPL(VSPUserClient, AsyncCallback)
{
    //kern_return_t ret = kIOReturnSuccess;

    VSPLog(LOG_PREFIX, "AsyncCallback called.\n");

    if (!action) {
        VSPLog(LOG_PREFIX, "AsyncCallback: action null pointer");
        return;
    }
        
    // Get back our data previously stored in OSAction.
    TVSPControllerData* request = (TVSPControllerData*) action->GetReference();
    TVSPControllerData response = {};
    
    // copy from request (event action) object
    memcpy(&response, request, VSP_UCD_SIZE);
    
    // create client response
    uint64_t asyncData[VSP_UCD_SIZE * 3] = {};
    memcpy(asyncData + 1, &response, VSP_UCD_SIZE);

    if (ivars->m_cbAction != nullptr) {
        // 3 is the 1 leading "type" message plus the two elements
        // of the TVSPControllerData.
        AsyncCompletion(ivars->m_cbAction, kIOReturnSuccess, asyncData, 3);
    }

    return;
}

// --------------------------------------------------------------------
// ExternalMethod(...)
//
kern_return_t VSPUserClient::ExternalMethod(
                uint64_t selector,
                IOUserClientMethodArguments* arguments,
                const IOUserClientMethodDispatch* dispatch,
                OSObject* target,
                void* reference)
{
    kern_return_t ret = kIOReturnSuccess;
    
    VSPLog(LOG_PREFIX, "ExternalMethod called.\n");

    if (selector < 1 || selector > vspLastCommand) {
        VSPLog(LOG_PREFIX, "Invalid method selector detected, skip.");
        return kIOReturnBadArgument;
    }
    
    // Set method dispatch descriptor
    dispatch = &uc_methods[selector];
    
    // This will call the functions as defined in the IOUserClientMethodDispatch.
    ret = super::ExternalMethod(selector, arguments, dispatch, target, reference);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "ExternalMethod dispatch failed. code=%d", ret);
        return kIOReturnBadArgument;
    }

    return ret;
}

// --------------------------------------------------------------------
// Called by VSPDriver root object
//
void VSPUserClient::setParent(VSPDriver* parent)
{
    if (ivars && !ivars->m_parent) {
        ivars->m_parent = parent;
    }
}

// --------------------------------------------------------------------
// Called by scheduleEvent()
// Dispatching action async
kern_return_t VSPUserClient::scheduleEvent()
{
    VSPLog(LOG_PREFIX, "scheduleEvent called.\n");
    uint64_t       currentTime = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);
    currentTime               += 10000000;
    const uint64_t leeway      = 1000000000;
    return ivars->m_eventSource->WakeAtTime(kIOTimerClockMonotonicRaw, currentTime, leeway);
}

// --------------------------------------------------------------------
// Called by scheduleEvent()
// Dispatching action async
void VSPUserClient::setClientStatus(void* data, uint32_t code, const char* message)
{
    VSPLog(LOG_PREFIX, "setClientStatus called.\n");

    TVSPControllerData* cd = (TVSPControllerData*) data;
    strncpy(cd->status.message, message, VSP_UCD_MESSAGE_SIZE);
    cd->status.code = kIOReturnSuccess;
}

// --------------------------------------------------------------------
// MARK: Static External Handlers
// --------------------------------------------------------------------
#define VSP_CHECK_PARAM_RETURN(f, p) \
{ \
    if (p == nullptr) { \
        VSPLog(LOG_PREFIX, f ": Invalid arguments detected"); \
        return kIOReturnBadArgument; \
    } \
}

// --------------------------------------------------------------------
// Called by ExternalMethod(...)
//
kern_return_t VSPUserClient::exSetCallback(OSObject* target, void* reference, IOUserClientMethodArguments* arguments)
{
    VSPLog(LOG_PREFIX, "exSetCallback called.\n");

    VSP_CHECK_PARAM_RETURN("exSetCallback", target);
    VSP_CHECK_PARAM_RETURN("exSetCallback", reference);
    VSP_CHECK_PARAM_RETURN("exSetCallback", arguments);

    /// - Tag: RegisterAsyncCallback_StoreCompletion
    if (arguments->completion == nullptr) {
        VSPLog(LOG_PREFIX, "exSetCallback: Got a null completion.");
        return kIOReturnBadArgument;
    }

    VSPUserClient* self = (VSPUserClient*) target;
    return self->setCallback(reference, arguments);
}

kern_return_t VSPUserClient::setCallback(void* reference, IOUserClientMethodArguments* arguments)
{
    VSPLog(LOG_PREFIX, "setCallback called.\n");

    // IOReturn ret = kIOReturnSuccess;
    TVSPControllerData* request = nullptr;
    TVSPControllerData response = {};
    
    // Save the completion for later. If not saved, then it
    // might be freed before the asychronous return.
    ivars->m_cbAction = arguments->completion;
    ivars->m_cbAction->retain();

    // All of this is returned synchronously. This is provided for the sake
    // of example. Generally a dext would want to return from an async method
    // as fast as possible.
    request = (TVSPControllerData*) arguments->structureInput->getBytesNoCopy();

    // Retain action memory for later work.
    void* evData = ivars->m_eventAction->GetReference();
    memcpy(evData, request, VSP_UCD_SIZE);

    response.context = VSPUserContext::vspContextPing;
    setClientStatus(&response.status, kIOReturnSuccess, "setCallback=OK");

    arguments->structureOutput = OSData::withBytes(&response, VSP_UCD_SIZE);
    

    return scheduleEvent();
}

// --------------------------------------------------------------------
// Called by ExternalMethod(...)
//
kern_return_t VSPUserClient::exLinkPorts(OSObject* target, void* reference, IOUserClientMethodArguments* arguments)
{
    VSPLog(LOG_PREFIX, "exLinkPorts called.\n");

    VSP_CHECK_PARAM_RETURN("exLinkPorts", target);
    VSP_CHECK_PARAM_RETURN("exLinkPorts", reference);
    VSP_CHECK_PARAM_RETURN("exLinkPorts", arguments);
    
    /// - Tag: RegisterAsyncCallback_StoreCompletion
    if (arguments->completion == nullptr) {
        VSPLog(LOG_PREFIX, "exSetCallback: Got a null completion.");
        return kIOReturnBadArgument;
    }

    VSPUserClient* self = (VSPUserClient*) target;
    return self->linkPorts(reference, arguments);
}

kern_return_t VSPUserClient::linkPorts(void* reference, IOUserClientMethodArguments* arguments)
{
    VSPLog(LOG_PREFIX, "linkPorts called.\n");

    // IOReturn ret = kIOReturnSuccess;
    TVSPControllerData* request = nullptr;
    TVSPControllerData response = {};
    
    // Save the completion for later. If not saved, then it
    // might be freed before the asychronous return.
    ivars->m_cbAction = arguments->completion;
    ivars->m_cbAction->retain();

    // All of this is returned synchronously. This is provided for the sake
    // of example. Generally a dext would want to return from an async method
    // as fast as possible.
    request = (TVSPControllerData*) arguments->structureInput->getBytesNoCopy();
    
    // Retain action memory for later work.
    void* evData = ivars->m_eventAction->GetReference();
    memcpy(evData, request, VSP_UCD_SIZE);

    VSPLog(LOG_PREFIX, "linkPorts: srcPort=%d tgtPort=%d.\n",
           request->parameter.portLink.sourceId,
           request->parameter.portLink.targetId);

    response.context = VSPUserContext::vspContextPort;
    setClientStatus(&response.status, kIOReturnSuccess, "linkPorts=OK");
    arguments->structureOutput = OSData::withBytes(&response, VSP_UCD_SIZE);
    
    return scheduleEvent();
}
