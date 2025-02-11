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

// Define all possible commands with its parameters and callback entry points.
const IOUserClientMethodDispatch uc_methods[vspLastCommand] = {
    [vspControlGetStatus] =
    {
        .function = VSP_METHOD(exGetStatus),
        .checkCompletionExists = true,
        .checkScalarInputCount = 0,
        .checkScalarOutputCount = 0,
        .checkStructureInputSize = VSP_UCD_SIZE,
        .checkStructureOutputSize = VSP_UCD_SIZE,
    },
    [vspControlGetPortList] =
    {
        .function = VSP_METHOD(exGetPortList),
        .checkCompletionExists = true,
        .checkScalarInputCount = 0,
        .checkScalarOutputCount = 0,
        .checkStructureInputSize = VSP_UCD_SIZE,
        .checkStructureOutputSize = VSP_UCD_SIZE,
    },
    [vspControlLinkPorts] =
    {
        .function = VSP_METHOD(exLinkPorts),
        .checkCompletionExists = true,
        .checkScalarInputCount = 0,
        .checkScalarOutputCount = 0,
        .checkStructureInputSize = VSP_UCD_SIZE,
        .checkStructureOutputSize = VSP_UCD_SIZE,
    },
    [vspControlUnlinkPorts] =
    {
        .function = VSP_METHOD(exUnlinkPorts),
        .checkCompletionExists = true,
        .checkScalarInputCount = 0,
        .checkScalarOutputCount = 0,
        .checkStructureInputSize = VSP_UCD_SIZE,
        .checkStructureOutputSize = VSP_UCD_SIZE,
    },
    [vspControlEnableChecks] =
    {
        .function = VSP_METHOD(exEnableChecks),
        .checkCompletionExists = true,
        .checkScalarInputCount = 0,
        .checkScalarOutputCount = 0,
        .checkStructureInputSize = VSP_UCD_SIZE,
        .checkStructureOutputSize = VSP_UCD_SIZE,
    },
    [vspControlEnableTrace] =
    {
        .function = VSP_METHOD(exEnableTrace),
        .checkCompletionExists = true,
        .checkScalarInputCount = 0,
        .checkScalarOutputCount = 0,
        .checkStructureInputSize = VSP_UCD_SIZE,
        .checkStructureOutputSize = VSP_UCD_SIZE,
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
    
    VSPLog(LOG_PREFIX, "init finished.\n");
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
        // 3 is the 1 leading "type" message plus the two elements of the TVSPControllerData.
        AsyncCompletion(ivars->m_cbAction, kIOReturnSuccess, asyncData, 3);
    }
    
    return;
}

// --------------------------------------------------------------------
// ExternalMethod(...)
//
kern_return_t VSPUserClient::ExternalMethod(uint64_t selector,
                                            IOUserClientMethodArguments* arguments,
                                            const IOUserClientMethodDispatch* dispatch,
                                            OSObject* target,
                                            void* reference)
{
    kern_return_t ret = kIOReturnSuccess;
    
    VSPLog(LOG_PREFIX, "ExternalMethod called.\n");
    
    if (selector < 1 || selector >= vspLastCommand) {
        VSPLog(LOG_PREFIX, "Invalid method selector detected, skip.");
        return kIOReturnBadArgument;
    }
    
    // Set method dispatch descriptor
    dispatch = &uc_methods[selector];
    target   = (target == nullptr ? this : target);
    reference= (reference == nullptr ? ivars : reference);

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
// Called by scheduleEvent() and dispatching action as async event
kern_return_t VSPUserClient::scheduleEvent()
{
    VSPLog(LOG_PREFIX, "scheduleEvent called.\n");
    uint64_t       currentTime = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);
    currentTime               += 100000000;
    const uint64_t leeway      = 1000000000;
    return ivars->m_eventSource->WakeAtTime(kIOTimerClockMonotonicRaw, currentTime, leeway);
}

// --------------------------------------------------------------------
// Set status response as result of a client request
//
void VSPUserClient::setClientStatus(void* data, uint32_t code, const char* message)
{
    VSPLog(LOG_PREFIX, "setClientStatus called. code=%d msg=%s\n", code, message);
    
    TVSPControllerData* cd = (TVSPControllerData*) data;
    strncpy(cd->status.message, message, VSP_UCD_MESSAGE_SIZE);
            cd->status.code   = kIOReturnSuccess;

    VSPLog(LOG_PREFIX, "setClientStatus finish.\n");
}

// --------------------------------------------------------------------
// Prepares async response
//
kern_return_t VSPUserClient::prepareResponse(void* reference, IOUserClientMethodArguments* arguments)
{
    TVSPControllerData* response = (TVSPControllerData*) reference;
    
    VSPLog(LOG_PREFIX, "prepareResponse called.\n");

    if (!response) {
        VSPLog(LOG_PREFIX, "prepareResponse Invalid argument 'reference' detected.\n");
        return kIOReturnBadArgument;
    }
    
    if (!arguments || !arguments->completion) {
        VSPLog(LOG_PREFIX, "prepareResponse Invalid argument 'arguments' detected.\n");
        return kIOReturnBadArgument;
    }

    // Save the completion for later. If not saved, then it
    // might be freed before the asychronous return.
    ivars->m_cbAction = arguments->completion;
    ivars->m_cbAction->retain();
        
    // Retain action memory for later work.
    void* evData = ivars->m_eventAction->GetReference();
    memcpy(evData, response, VSP_UCD_SIZE);

    // directly response
    arguments->structureOutput = OSData::withBytes(response, VSP_UCD_SIZE);

    VSPLog(LOG_PREFIX, "prepareResponse finish.\n");
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// MARK: Static External Handlers
// --------------------------------------------------------------------

#define VSP_CHECK_PARAM_RETURN(f, p) \
{ \
    if (p == nullptr) { \
        VSPLog(LOG_PREFIX, "%s: Invalid argument '" f "' detected", __func__); \
        return kIOReturnBadArgument; \
    } \
}

// --------------------------------------------------------------------
// Called by ExternalMethod(...)
//
kern_return_t VSPUserClient::exGetStatus(OSObject* target, void* reference, IOUserClientMethodArguments* arguments)
{
    VSPLog(LOG_PREFIX, "exGetStatus called.\n");

    VSP_CHECK_PARAM_RETURN("target", target);
    VSP_CHECK_PARAM_RETURN("arguments", arguments);
    VSP_CHECK_PARAM_RETURN("completion", arguments->completion);

    VSPUserClient* self = (VSPUserClient*) target;
    return self->getStatus(reference, arguments);
}

kern_return_t VSPUserClient::getStatus(void* reference, IOUserClientMethodArguments* arguments)
{
    TVSPControllerData* request = nullptr;
    TVSPControllerData response = {};
    kern_return_t ret;

    VSPLog(LOG_PREFIX, "getStatus called.\n");

    // Generally a dext would want to return from an async method as fast as possible.
    request = (TVSPControllerData*) arguments->structureInput->getBytesNoCopy();
    
    setClientStatus(&response.status, kIOReturnSuccess, "getStatus:OK");

    response.context   = VSPUserContext::vspContextPing;
    response.command   = request->command;
    response.parameter = request->parameter;

    if ((ret = prepareResponse(&response, arguments)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "getStatus preprare response failed. code=%d\n", ret);
        return ret;
    }
     
    VSPLog(LOG_PREFIX, "getStatus finish.\n");

    return scheduleEvent();
}

// --------------------------------------------------------------------
// Called by ExternalMethod(...)
//
kern_return_t VSPUserClient::exLinkPorts(OSObject* target, void* reference, IOUserClientMethodArguments* arguments)
{
    VSPLog(LOG_PREFIX, "exLinkPorts called.\n");
    
    VSP_CHECK_PARAM_RETURN("target", target);
    VSP_CHECK_PARAM_RETURN("arguments", arguments);
    VSP_CHECK_PARAM_RETURN("completion", arguments->completion);

    VSPUserClient* self = (VSPUserClient*) target;
    return self->linkPorts(reference, arguments);
}

kern_return_t VSPUserClient::linkPorts(void* reference, IOUserClientMethodArguments* arguments)
{
    TVSPControllerData* request = nullptr;
    TVSPControllerData response = {};
    kern_return_t ret;

    VSPLog(LOG_PREFIX, "linkPorts called.\n");
    
    // Generally a dext would want to return from an async method as fast as possible.
    request = (TVSPControllerData*) arguments->structureInput->getBytesNoCopy();
    
    setClientStatus(&response.status, kIOReturnSuccess, "getStatus:OK");

    response.context   = VSPUserContext::vspContextPing;
    response.command   = request->command;
    response.parameter = request->parameter;

    if ((ret = prepareResponse(&response, arguments)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "getStatus preprare response failed. code=%d\n", ret);
        return ret;
    }

    VSPLog(LOG_PREFIX, "linkPorts finish.\n");

    return scheduleEvent();
}

// --------------------------------------------------------------------
// Called by ExternalMethod(...)
//
kern_return_t VSPUserClient::exGetPortList(OSObject* target, void* reference, IOUserClientMethodArguments* arguments)
{
    VSPLog(LOG_PREFIX, "exGetPortList called.\n");
    
    VSP_CHECK_PARAM_RETURN("target", target);
    VSP_CHECK_PARAM_RETURN("arguments", arguments);
    VSP_CHECK_PARAM_RETURN("completion", arguments->completion);

    VSPUserClient* self = (VSPUserClient*) target;
    return self->getPortList(reference, arguments);
}

kern_return_t VSPUserClient::getPortList(void* reference, IOUserClientMethodArguments* arguments)
{
    TVSPControllerData* request = nullptr;
    TVSPControllerData response = {};
    kern_return_t ret;

    VSPLog(LOG_PREFIX, "getPortList called.\n");
    
    // Generally a dext would want to return from an async method as fast as possible.
    request = (TVSPControllerData*) arguments->structureInput->getBytesNoCopy();
    
    setClientStatus(&response.status, kIOReturnSuccess, "getStatus:OK");

    response.context   = VSPUserContext::vspContextPort;
    response.command   = request->command;
    response.parameter = request->parameter;

    if ((ret = prepareResponse(&response, arguments)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "getStatus preprare response failed. code=%d\n", ret);
        return ret;
    }

    VSPLog(LOG_PREFIX, "getPortList finish.\n");

    return scheduleEvent();
}

// --------------------------------------------------------------------
// Called by ExternalMethod(...)
//
kern_return_t VSPUserClient::exUnlinkPorts(OSObject* target, void* reference, IOUserClientMethodArguments* arguments)
{
    VSPLog(LOG_PREFIX, "exUnlinkPorts called.\n");

    VSP_CHECK_PARAM_RETURN("target", target);
    VSP_CHECK_PARAM_RETURN("arguments", arguments);
    VSP_CHECK_PARAM_RETURN("completion", arguments->completion);

    VSPUserClient* self = (VSPUserClient*) target;
    return self->unlinkPorts(reference, arguments);
}

kern_return_t VSPUserClient::unlinkPorts(void* reference, IOUserClientMethodArguments* arguments)
{
    TVSPControllerData* request = nullptr;
    TVSPControllerData response = {};
    kern_return_t ret;

    VSPLog(LOG_PREFIX, "unlinkPorts called.\n");
    
    // Generally a dext would want to return from an async method as fast as possible.
    request = (TVSPControllerData*) arguments->structureInput->getBytesNoCopy();
    
    setClientStatus(&response.status, kIOReturnSuccess, "getStatus:OK");

    response.context   = VSPUserContext::vspContextPort;
    response.command   = request->command;
    response.parameter = request->parameter;

    if ((ret = prepareResponse(&response, arguments)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "getStatus preprare response failed. code=%d\n", ret);
        return ret;
    }

    VSPLog(LOG_PREFIX, "unlinkPorts finish.\n");

    return scheduleEvent();
}

// --------------------------------------------------------------------
// Called by ExternalMethod(...)
//
kern_return_t VSPUserClient::exEnableChecks(OSObject* target, void* reference, IOUserClientMethodArguments* arguments)
{
    VSPLog(LOG_PREFIX, "exEnableChecks called.\n");
    
    VSP_CHECK_PARAM_RETURN("target", target);
    VSP_CHECK_PARAM_RETURN("arguments", arguments);
    VSP_CHECK_PARAM_RETURN("completion", arguments->completion);

    VSPUserClient* self = (VSPUserClient*) target;
    return self->enableChecks(reference, arguments);
}

kern_return_t VSPUserClient::enableChecks(void* reference, IOUserClientMethodArguments* arguments)
{
    TVSPControllerData* request = nullptr;
    TVSPControllerData response = {};
    kern_return_t ret;

    VSPLog(LOG_PREFIX, "enableChecks called.\n");
    
    // Generally a dext would want to return from an async method as fast as possible.
    request = (TVSPControllerData*) arguments->structureInput->getBytesNoCopy();
    
    setClientStatus(&response.status, kIOReturnSuccess, "getStatus:OK");

    response.context   = VSPUserContext::vspContextPort;
    response.command   = request->command;
    response.parameter = request->parameter;

    if ((ret = prepareResponse(&response, arguments)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "getStatus preprare response failed. code=%d\n", ret);
        return ret;
    }

    VSPLog(LOG_PREFIX, "enableChecks finish.\n");

    return scheduleEvent();
}

// --------------------------------------------------------------------
// Called by ExternalMethod(...)
//
kern_return_t VSPUserClient::exEnableTrace(OSObject* target, void* reference, IOUserClientMethodArguments* arguments)
{
    VSPLog(LOG_PREFIX, "exEnableTrace called.\n");
    
    VSP_CHECK_PARAM_RETURN("target", target);
    VSP_CHECK_PARAM_RETURN("arguments", arguments);
    VSP_CHECK_PARAM_RETURN("completion", arguments->completion);

    VSPUserClient* self = (VSPUserClient*) target;
    return self->enableTrace(reference, arguments);
}

kern_return_t VSPUserClient::enableTrace(void* reference, IOUserClientMethodArguments* arguments)
{
    TVSPControllerData* request = nullptr;
    TVSPControllerData response = {};
    kern_return_t ret;

    VSPLog(LOG_PREFIX, "enableTrace called.\n");
    
    // Generally a dext would want to return from an async method as fast as possible.
    request = (TVSPControllerData*) arguments->structureInput->getBytesNoCopy();
    
    setClientStatus(&response.status, kIOReturnSuccess, "getStatus:OK");

    response.context   = VSPUserContext::vspContextPort;
    response.command   = request->command;
    response.parameter = request->parameter;

    if ((ret = prepareResponse(&response, arguments)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "getStatus preprare response failed. code=%d\n", ret);
        return ret;
    }

    VSPLog(LOG_PREFIX, "enableTrace finish.\n");

    return scheduleEvent();
}
