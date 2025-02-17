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
#include <DriverKit/IOMemoryMap.h>
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

#define VSP_METHOD(x) ((IOUserClientMethodFunction) &VSPUserClient::x)

#define VSP_CHECK_PARAM_RETURN(f, p) \
{ \
    if (p == nullptr) { \
        VSPLog(LOG_PREFIX, "%s: Invalid argument '" f "' detected", __func__); \
        return kIOReturnBadArgument; \
    } \
}

// Call VSPUserClient instance method
#define VSP_HANDLER_CALL(target, handler) \
{ \
    VSPUserClient* self = (VSPUserClient*) target; \
    return self->handler(reference, arguments); \
}

// Implements the static method for ExternalMethod dispatch
#define VSP_IMPL_EX_METHOD(prefix, name, handler) \
VSPUserClient::name(OSObject* target, void* reference, IOUserClientMethodArguments* arguments) \
{ \
    VSPLog(LOG_PREFIX, prefix " called.\n"); \
    VSP_HANDLER_CALL(target, handler) \
}

#ifdef DEBUG
#define VSP_DUMP_DATA(data) dump_ctrl_data(data)
static inline void dump_ctrl_data(const TVSPControllerData* data)
{
    VSPLog(LOG_PREFIX, "Data ------------------------------------------\n");
    VSPLog(LOG_PREFIX, "Data.context..........: %d\n", data->context);
    VSPLog(LOG_PREFIX, "Data.command..........: %d\n", data->command);
    VSPLog(LOG_PREFIX, "Data.status.code......: %d\n", data->status.code);
    VSPLog(LOG_PREFIX, "Data.status.flags.....: 0x%llx\n", data->status.flags);
    VSPLog(LOG_PREFIX, "Data.parameter.flags..: 0x%llx\n", data->parameter.flags);
    VSPLog(LOG_PREFIX, "Data.p.portlink.source: %d\n", data->parameter.link.source);
    VSPLog(LOG_PREFIX, "Data.p.portlink.target: %d\n", data->parameter.link.target);
    VSPLog(LOG_PREFIX, "Data.ports.count......: %d\n", data->ports.count);
    for (uint8_t i = 0; i < data->ports.count && i < MAX_SERIAL_PORTS; i++) {
        VSPLog(LOG_PREFIX, "\tPort item #%d: %d\n", i, data->ports.list[i]);
    }
    VSPLog(LOG_PREFIX, "Data.links.count......: %d\n", data->links.count);
    for (uint8_t i = 0; i < data->links.count && i < MAX_SERIAL_PORTS; i++) {
        VSPLog(LOG_PREFIX, "\tPort Link #%d: %d <-> %d\n", //
               (uint8_t)(data->links.list[i] >> 16) & 0x000000ff,
               (uint8_t)(data->links.list[i] >> 8) & 0x000000ff,
               (uint8_t)(data->links.list[i]) & 0x000000ff);
    }
}
#else
#define VSP_DUMP_DATA(data)
#endif

#define VSP_INIT_RESPONSE(request,flags) \
do { \
    VSP_DUMP_DATA(request); \
    memcpy(&response, request, VSP_UCD_SIZE); \
    set_ctlr_status(&response, kIOReturnSuccess, flags); \
} while(0)

struct VSPUserClient_IVars {
    IOService* m_provider = nullptr;
    VSPDriver* m_parent = nullptr;
    IODispatchQueue* m_eventQueue = nullptr;
    IOTimerDispatchSource* m_eventSource = nullptr;
    OSAction* m_eventAction = nullptr;
    OSAction* m_cbAction = nullptr;
};

// Define all possible commands with its parameters and callback entry points.
const IOUserClientMethodDispatch uc_methods[vspLastCommand] = {
    [vspControlPingPong] =
    {
        .function = VSP_METHOD(exGetStatus),
        .checkCompletionExists = true,
        .checkScalarInputCount = 0,
        .checkScalarOutputCount = 0,
        .checkStructureInputSize = VSP_UCD_SIZE,
        .checkStructureOutputSize = VSP_UCD_SIZE,
    },
    [vspControlGetStatus] =
    {
        .function = VSP_METHOD(exGetStatus),
        .checkCompletionExists = true,
        .checkScalarInputCount = 0,
        .checkScalarOutputCount = 0,
        .checkStructureInputSize = VSP_UCD_SIZE,
        .checkStructureOutputSize = VSP_UCD_SIZE,
    },
    [vspControlCreatePort] =
    {
        .function = VSP_METHOD(exCreatePort),
        .checkCompletionExists = true,
        .checkScalarInputCount = 0,
        .checkScalarOutputCount = 0,
        .checkStructureInputSize = VSP_UCD_SIZE,
        .checkStructureOutputSize = VSP_UCD_SIZE,
    },
    [vspControlRemovePort] =
    {
        .function = VSP_METHOD(exRemovePort),
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
    [vspControlGetLinkList] =
    {
        .function = VSP_METHOD(exGetLinkList),
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

static inline void set_ctlr_status(TVSPControllerData* data, uint32_t code, uint64_t flags)
{
    data->context = (code == kIOReturnSuccess ? vspContextResult : vspContextError);
    data->status.code = code;
    data->status.flags = flags;
}

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
    
    ret = IODispatchQueue::Create("kVSPUserClientQueue", 0, 0, &ivars->m_eventQueue);
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
    
    ret = CreateActionAsyncCallback(VSP_UCD_SIZE, &ivars->m_eventAction);
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
// CopyClientMemoryForType_Impl(uint64_t type, uint64_t *options, IOMemoryDescriptor **memory)
// Allocate space for UC
//
kern_return_t IMPL(VSPUserClient, CopyClientMemoryForType)
{
    kern_return_t ret = kIOReturnSuccess;
    
    VSPLog(LOG_PREFIX, "CopyClientMemoryForType called. type=%llu optionsptr=0x%llx\n",
           type, (uint64_t)options);
    
    if (type >= 0 && type < vspLastCommand)
    {
        IOBufferMemoryDescriptor* md;
        ret = IOBufferMemoryDescriptor::Create(kIOMemoryDirectionInOut, //
                                               VSP_UCD_SIZE    /* capacity */, //
                                               0               /* alignment */, //
                                               &md);
        if (ret != kIOReturnSuccess || !md) {
            VSPLog(LOG_PREFIX, "CopyClientMemoryForType: IOBufferMemoryDescriptor::Create failed: 0x%x", ret);
            return ret;
        }
        
        // returned with refcount 1
        *memory = md;
    }
    else
    {
        VSPLog(LOG_PREFIX, "CopyClientMemoryForType super call.\n");
        ret = this->CopyClientMemoryForType(type, options, memory, SUPERDISPATCH);
    }
    
    VSPLog(LOG_PREFIX, "CopyClientMemoryForType complete.\n");
    return ret;
}

// --------------------------------------------------------------------
// AsyncCallback_IMPL(...)
// Event raised by IOTimerDispatchSource::TimerOccurred event
//
void IMPL(VSPUserClient, AsyncCallback)
{
    VSPLog(LOG_PREFIX, "AsyncCallback called.\n");
    
    if (!action) {
        VSPLog(LOG_PREFIX, "AsyncCallback: Event action NULL pointer!");
        return;
    }
    if (!ivars->m_cbAction) {
        VSPLog(LOG_PREFIX, "AsyncCallback: Client callback action NULL pointer!");
        return;
    }

    // Get back our data previously stored in OSAction.
    TVSPControllerData* evd = (TVSPControllerData*) action->GetReference();

    // Get function response data
    TVSPControllerData response = {};
    memcpy(&response, evd, VSP_UCD_SIZE);
   
    // Debug !! 
    VSP_DUMP_DATA(&response);

    // Async callback message to user client
    uint64_t* message = IONewZero(uint64_t, 4);
    message[0] = MAGIC_CONTROL;
    message[1] = VSP_UCD_SIZE | 0x001f;
    message[2] = response.context;
    message[3] = response.command;

    // dispatch message back to user client
    AsyncCompletion(ivars->m_cbAction, response.status.code, message, 4);
    IOSafeDeleteNULL(message, uint64_t, 4);

    VSPLog(LOG_PREFIX, "AsyncCallback complete.");
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
    
    VSPLog(LOG_PREFIX, "ExternalMethod called. sel=%llu arg=0x%llx disp=0x%llx tgt=0x%llx ref=0x%llx\n",
           selector,
           (uint64_t)arguments,
           (uint64_t)dispatch,
           (uint64_t)target,
           (uint64_t)reference);

    if (selector >= vspLastCommand) {
        VSPLog(LOG_PREFIX, "Invalid method selector detected, skip.");
        return kIOReturnBadArgument;
    }
    
    // Set method dispatch descriptor
    dispatch = &uc_methods[selector];
    target   = (target == nullptr ? this : target);
    reference= (reference == nullptr ? ivars : reference);

    VSP_CHECK_PARAM_RETURN("arguments", arguments);  \
    VSP_CHECK_PARAM_RETURN("completion", arguments->completion); \
    VSP_CHECK_PARAM_RETURN("target", target); \

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
// Remove link to VSPDriver instance
//
void VSPUserClient::unlinkParent()
{
    VSPLog(LOG_PREFIX, "unlinkParent called.\n");
    
    ivars->m_parent = nullptr;
}

// --------------------------------------------------------------------
// Called by scheduleEvent() and dispatching action as async event
kern_return_t VSPUserClient::scheduleEvent(void* reference, IOUserClientMethodArguments* arguments)
{
    TVSPControllerData* response = (TVSPControllerData*) reference;
    IOReturn ret;
 
    VSPLog(LOG_PREFIX, "scheduleEvent called.\n");
    
    if ((ret = prepareResponse(response, arguments)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "scheduleEvent: Preprare response failed. code=%d\n", ret);
        set_ctlr_status(response, ret, 0xea000001);
    }
    
    uint64_t       currentTime = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);
    currentTime               += 10000000;
    const uint64_t leeway      = 1000000000;
    return ivars->m_eventSource->WakeAtTime(kIOTimerClockMonotonicRaw, currentTime, leeway);
}

// --------------------------------------------------------------------
// Prepares async response
//
kern_return_t VSPUserClient::prepareResponse(void* reference, IOUserClientMethodArguments* arguments)
{
    const TVSPControllerData* response = reinterpret_cast<TVSPControllerData*>(reference);
    IOMemoryMap* outputMap = nullptr;
    uint8_t* outputPtr;
    uint64_t length;
    IOReturn ret;
    
    VSPLog(LOG_PREFIX, "prepareResponse called.\n");

    if (!response) {
        VSPLog(LOG_PREFIX, "prepareResponse: Invalid argument 'reference' detected.\n");
        return kIOReturnBadArgument;
    }

    if (!arguments || !arguments->completion) {
        VSPLog(LOG_PREFIX, "prepareResponse: Invalid argument 'arguments' detected.\n");
        return kIOReturnBadArgument;
    }
 
    VSP_DUMP_DATA(response);

    // Memory is not passed from the caller into the dext.
    // The dext needs to create its own OSData to hold this
    // information in order to pass it back to the caller.
    if (arguments->structureOutputDescriptor != nullptr)
    {
        if (VSP_UCD_SIZE > arguments->structureOutputMaximumSize)
        {
            VSPLog(LOG_PREFIX,
                   "prepareResponse: Required output size of %lu"
                   "is larger than the given maximum UC size of %llu. Failing.",
                   VSP_UCD_SIZE, arguments->structureOutputMaximumSize);
            return kIOReturnNoSpace;
        }

        ret = arguments->structureOutputDescriptor->CreateMapping(0, 0, 0, 0, 0, &outputMap);
        if (ret != kIOReturnSuccess || !outputMap) {
            VSPLog(LOG_PREFIX, "prepareResponse: UC output CreateMapping failed. code=%d\n", ret);
        }
        else {
            outputPtr = (uint8_t*) outputMap->GetAddress();
            length    = outputMap->GetLength();
            
            VSPLog(LOG_PREFIX, "prepareResponse: Share result to UC. length: %lld addr: 0x%llx\n",
                   length, (uint64_t) outputPtr);
            
            // Copy the data from DataStruct over and then fill the rest with zeroes.
            memcpy(outputPtr, response, (length < VSP_UCD_SIZE ? length : VSP_UCD_SIZE));
            memset(outputPtr + VSP_UCD_SIZE, 0, length - VSP_UCD_SIZE);
            OSSafeReleaseNULL(outputMap);
        }
    }
    else
    {
        arguments->structureOutput = OSData::withBytes(response, VSP_UCD_SIZE);
    }
    
    // Save the completion for later. If not saved, then it
    // might be freed before the asychronous return.
    ivars->m_cbAction = arguments->completion;
    ivars->m_cbAction->retain();
        
    // Retain action memory for later work.
    void* evData = ivars->m_eventAction->GetReference();
    memcpy(evData, response, VSP_UCD_SIZE);
    
    VSPLog(LOG_PREFIX, "prepareResponse finish.\n");
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// MARK: Static External Handlers
// --------------------------------------------------------------------

inline static IOReturn toRequest(IOUserClientMethodArguments* arguments, TVSPControllerData* request)
{
    IOReturn ret;
    IOMemoryMap* inputMap = nullptr;
    void* buffer = nullptr;
    uint64_t size = 0;
    
    if (!request) {
        return kIOReturnBadArgument;
    }
    
    if (arguments->structureInput != nullptr)
    {
        size = arguments->structureInput->getLength();
        buffer = (void*)arguments->structureInput->getBytesNoCopy();
        memcpy(request, buffer, (size < VSP_UCD_SIZE ? size : VSP_UCD_SIZE));
    }
    else if (arguments->structureInputDescriptor != nullptr)
    {
        ret = arguments->structureInputDescriptor->CreateMapping(0, 0, 0, 0, 0, &inputMap);
        if (ret != kIOReturnSuccess || !inputMap)
        {
            VSPLog(LOG_PREFIX, "toRequest: Failed to create mapping for descriptor with error: 0x%08x", ret);
            return kIOReturnBadArgument;
        }
        size = inputMap->GetLength();
        buffer = (void*) inputMap->GetAddress();
        memcpy(request, buffer, (size < VSP_UCD_SIZE ? size : VSP_UCD_SIZE));
        OSSafeReleaseNULL(inputMap);
    }
    else
    {
        VSPLog(LOG_PREFIX, "toRequest: Both structureInput and structureInputDescriptor were null.");
        return kIOReturnBadArgument;
    }
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// Return status of VSPDriver instance
//
kern_return_t VSP_IMPL_EX_METHOD("exGetStatus", exGetStatus, getStatus)
kern_return_t VSPUserClient::getStatus(void* reference, IOUserClientMethodArguments* arguments)
{
    TVSPControllerData request = {};
    TVSPControllerData response = {};
    uint8_t portCount = 0;
    uint8_t linkCount = 0;
    uint8_t portList[MAX_SERIAL_PORTS] = {};
    uint64_t linkList[MAX_PORT_LINKS] = {};
    kern_return_t ret;

    VSPLog(LOG_PREFIX, "getStatus called.\n");
    
    if ((ret = toRequest(arguments, &request)) != kIOReturnSuccess) {
        set_ctlr_status(&response, ret, 0xdd000000);
        goto finish;
    }
    
    VSP_INIT_RESPONSE(&request, MAGIC_CONTROL);

    if ((request.status.flags & MAGIC_CONTROL) != MAGIC_CONTROL) {
        set_ctlr_status(&response, kIOReturnInvalid, 0xee000000);
        goto finish;
    }
    
    if ((ret = ivars->m_parent->getPortCount(&portCount)) != kIOReturnSuccess) {
        set_ctlr_status(&response, ret, 0xfe000001);
        goto finish;
    }
    else if (portCount) {
        if ((ret = ivars->m_parent->getPortList(portList, portCount)) != kIOReturnSuccess) {
            set_ctlr_status(&response, ret, 0xfe000003);
            goto finish;
        }
        for(uint8_t i = 0; i < portCount && i < MAX_SERIAL_PORTS; i++) {
            response.ports.list[i] = portList[i];
        }
        response.ports.count = portCount;
        response.parameter.flags |= BIT(1);
    }

    if ((ret = ivars->m_parent->getPortLinkCount(&linkCount)) != kIOReturnSuccess) {
        set_ctlr_status(&response, ret, 0xfe000002);
        goto finish;
    }
    else if (linkCount) {
        if ((ret = ivars->m_parent->getPortLinkList(linkList, linkCount)) != kIOReturnSuccess) {
            set_ctlr_status(&response, ret, 0xfe000004);
            goto finish;
        }
        for(uint8_t i = 0; i < linkCount && i < MAX_PORT_LINKS; i++) {
            response.links.list[i] = linkList[i];
        }
        response.links.count = linkCount;
        response.parameter.flags |= BIT(2);
    }
    
    VSPLog(LOG_PREFIX, "getStatus finish.\n");
    
finish:
    return scheduleEvent(&response, arguments);
}

// --------------------------------------------------------------------
// Create new serial port instance
//
kern_return_t VSP_IMPL_EX_METHOD("exCreatePort", exCreatePort, createPort)
kern_return_t VSPUserClient::createPort(void* reference, IOUserClientMethodArguments* arguments)
{
    TVSPControllerData response = {};
    TVSPControllerData request = {};
    kern_return_t ret;

    VSPLog(LOG_PREFIX, "createPort called.\n");

    if ((ret = toRequest(arguments, &request)) != kIOReturnSuccess) {
        set_ctlr_status(&response, ret, 0xdd000000);
        goto finish;
    }

    VSP_INIT_RESPONSE(&request, MAGIC_CONTROL);

    if ((request.status.flags & MAGIC_CONTROL) != MAGIC_CONTROL) {
        set_ctlr_status(&response, kIOReturnInvalid, 0xee000000);
        goto finish;
    }

    if ((ret = ivars->m_parent->createPort())!= kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "createPort: Parent createPort failed. code=%d\n", ret);
        set_ctlr_status(&response, ret, 0xfa000001);
    }
    else if (getPortListHelper(&response) == kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "createPort finish.\n");
    }
    
finish:
    return scheduleEvent(&response, arguments);
}

// --------------------------------------------------------------------
// Remove serial port instance
//
kern_return_t VSP_IMPL_EX_METHOD("exRemovePort", exRemovePort, removePort)
kern_return_t VSPUserClient::removePort(void* reference, IOUserClientMethodArguments* arguments)
{
    kern_return_t ret;
    TVSPControllerData response = {};
    TVSPControllerData request = {};
    uint8_t portId = 0;

    VSPLog(LOG_PREFIX, "removePort called.\n");

    if ((ret = toRequest(arguments, &request)) != kIOReturnSuccess) {
        set_ctlr_status(&response, ret, 0xdd000000);
        goto finish;
    }

    VSP_INIT_RESPONSE(&request, MAGIC_CONTROL);

    if ((request.status.flags & MAGIC_CONTROL) != MAGIC_CONTROL) {
        set_ctlr_status(&response, kIOReturnInvalid, 0xee000000);
        goto finish;
    }

    portId = request.parameter.link.source;
    if ((ret = ivars->m_parent->removePort(portId))!= kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "removePort: Parent removePort failed. code=%d\n", ret);
        set_ctlr_status(&response, ret, 0xfa000002);
    }
    else if (getPortListHelper(&response) == kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "removePort finish.\n");
    }

finish:
    return scheduleEvent(&response, arguments);
}

// --------------------------------------------------------------------
// Return active port list
//
kern_return_t VSP_IMPL_EX_METHOD("exGetPortList", exGetPortList, getPortList)
kern_return_t VSPUserClient::getPortList(void* reference, IOUserClientMethodArguments* arguments)
{
    kern_return_t ret;
    TVSPControllerData response = {};
    TVSPControllerData request = {};

    VSPLog(LOG_PREFIX, "getPortList called.\n");

    if ((ret = toRequest(arguments, &request)) != kIOReturnSuccess) {
        set_ctlr_status(&response, ret, 0xdd000000);
        goto finish;
    }

    VSP_INIT_RESPONSE(&request, MAGIC_CONTROL);

    if ((request.status.flags & MAGIC_CONTROL) != MAGIC_CONTROL) {
        set_ctlr_status(&response, kIOReturnInvalid, 0xee000000);
        goto finish;
    }

    if (getPortListHelper(&response) == kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "getPortList finish.\n");
    }

finish:
    return scheduleEvent(&response, arguments);
}

kern_return_t VSPUserClient::getPortListHelper(void* reference)
{
    TVSPControllerData* response = reinterpret_cast<TVSPControllerData*>(reference);
    kern_return_t ret;
    uint8_t* list = nullptr;
    uint8_t count = 0;
    
    if ((ret = ivars->m_parent->getPortCount(&count)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "getPortListHelper: parent getPortCount failed. code=%d\n", ret);
        set_ctlr_status(response, ret, 0xfc000001);
    }

    if (count == 0) {
        /* be quiet to caller */
        VSPLog(LOG_PREFIX, "getPortListHelper: No serial ports available.\n");
        response->ports.count = 0;
    }
    else if (!(list = IONewZero(uint8_t, count))) {
        VSPLog(LOG_PREFIX, "getPortListHelper: Out of memory.\n");
        set_ctlr_status(response, ret, 0xfc0000ff);
    }
    else {
        if ((ret = ivars->m_parent->getPortList(list, count)) != kIOReturnSuccess) {
            VSPLog(LOG_PREFIX, "getPortListHelper: parent getPortList failed. code=%d\n", ret);
            set_ctlr_status(response, ret, 0xfc000002);
        }
        else {
            for (uint8_t i = 0; i < count; i++) {
                response->ports.list[i] = list[i];
            }
            response->ports.count = count;
        }
        IOSafeDeleteNULL(list, uint8_t, count);
    }
    
    
    return response->status.code;
}

// --------------------------------------------------------------------
// Return active port link list
//
kern_return_t VSP_IMPL_EX_METHOD("exGetLinkList", exGetLinkList, getLinkList)
kern_return_t VSPUserClient::getLinkList(void* reference, IOUserClientMethodArguments* arguments)
{
    kern_return_t ret;
    TVSPControllerData response = {};
    TVSPControllerData request = {};

    VSPLog(LOG_PREFIX, "getLinkList called.\n");

    if ((ret = toRequest(arguments, &request)) != kIOReturnSuccess) {
        set_ctlr_status(&response, ret, 0xdd000000);
        goto finish;
    }

    VSP_INIT_RESPONSE(&request, MAGIC_CONTROL);

    if ((request.status.flags & MAGIC_CONTROL) != MAGIC_CONTROL) {
        set_ctlr_status(&response, kIOReturnInvalid, 0xee000000);
        goto finish;
    }

    if (getLinkListHelper(&response) == kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "getLinkList finish.\n");
    }

finish:
    return scheduleEvent(&response, arguments);
}

kern_return_t VSPUserClient::getLinkListHelper(void* reference)
{
    TVSPControllerData* response = reinterpret_cast<TVSPControllerData*>(reference);
    kern_return_t ret;
    uint64_t* list = nullptr;
    uint8_t count = 0;

    if ((ret = getPortListHelper(reference)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "getLinkListHelper: getPortListHelper failed. code=%d\n", ret);
        set_ctlr_status(response, ret, 0xd000000a);
    }
    else if ((ret = ivars->m_parent->getPortLinkCount(&count)) != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "getLinkListHelper: parent getPortLinkCount failed. code=%d\n", ret);
        set_ctlr_status(response, ret, 0xd0000001);
    }
    else if (count == 0) {
        /* be quiet to caller */
        VSPLog(LOG_PREFIX, "getLinkListHelper: No port links available.\n");
        response->links.count = 0;
    }
    else if (!(list = IONewZero(uint64_t, count))) {
        VSPLog(LOG_PREFIX, "getLinkListHelper: Out of memory.\n");
        set_ctlr_status(response, ret, 0xd00000ff);
    }
    else {
        if ((ret = ivars->m_parent->getPortLinkList(list, count)) != kIOReturnSuccess) {
            VSPLog(LOG_PREFIX, "getLinkListHelper: parent getPortLinkList failed. code=%d\n", ret);
            set_ctlr_status(response, ret, 0xd0000002);
        }
        else {
            for (uint8_t i = 0; i < count; i++) {
                response->links.list[i] = list[i];
            }
            response->links.count = count;
        }
        IOSafeDeleteNULL(list, uint64_t, count);
    }
    
    return response->status.code;
}

// --------------------------------------------------------------------
// Link two serial ports together
//
kern_return_t VSP_IMPL_EX_METHOD("exLinkPorts", exLinkPorts, linkPorts)
kern_return_t VSPUserClient::linkPorts(void* reference, IOUserClientMethodArguments* arguments)
{
    kern_return_t ret;
    TVSPControllerData response = {};
    TVSPControllerData request = {};
    uint8_t sid, tid;
    void* link = nullptr;

    VSPLog(LOG_PREFIX, "linkPorts called.\n");

    if ((ret = toRequest(arguments, &request)) != kIOReturnSuccess) {
        set_ctlr_status(&response, ret, 0xdd000000);
        goto finish;
    }

    VSP_INIT_RESPONSE(&request, MAGIC_CONTROL);

    if ((request.status.flags & MAGIC_CONTROL) != MAGIC_CONTROL) {
        set_ctlr_status(&response, kIOReturnInvalid, 0xee000000);
        goto finish;
    }

    sid = request.parameter.link.source;
    tid = request.parameter.link.target;
    ret = ivars->m_parent->createPortLink(sid, tid, &link);
    if (ret != kIOReturnSuccess) {
        VSPLog(LOG_PREFIX, "linkPorts: parent createPortLink failed. code=%d\n", ret);
        set_ctlr_status(&response, ret, 0xfa000001);
    }
    else if (link) {
        if (getLinkListHelper(&response) == kIOReturnSuccess) {
            VSPLog(LOG_PREFIX, "linkPorts finish.\n");
        }
    }


finish:
    return scheduleEvent(&response, arguments);
}

// --------------------------------------------------------------------
// Unlink prior linked ports
//
kern_return_t VSP_IMPL_EX_METHOD("exUnlinkPorts", exUnlinkPorts, unlinkPorts)
kern_return_t VSPUserClient::unlinkPorts(void* reference, IOUserClientMethodArguments* arguments)
{
    kern_return_t ret;
    TVSPControllerData response = {};
    TVSPControllerData request = {};
    uint8_t sid, tid;
    void* link = nullptr;

    VSPLog(LOG_PREFIX, "unlinkPorts called.\n");

    if ((ret = toRequest(arguments, &request)) != kIOReturnSuccess) {
        set_ctlr_status(&response, ret, 0xdd000000);
        goto finish;
    }
        
    VSP_INIT_RESPONSE(&request, MAGIC_CONTROL);

    if ((request.status.flags & MAGIC_CONTROL) != MAGIC_CONTROL) {
        set_ctlr_status(&response, kIOReturnInvalid, 0xee000000);
        goto finish;
    }

    sid = request.parameter.link.source;
    tid = request.parameter.link.target;
    ret = ivars->m_parent->getPortLinkByPorts(sid, tid, &link);
    if (ret != kIOReturnSuccess) {
        set_ctlr_status(&response, ret, 0xfb000001);
    }
    else {
        TVSPLinkItem* item = reinterpret_cast<TVSPLinkItem*>(link);
        VSPLog(LOG_PREFIX, "unlinkPorts: remove src=%d tgt=%d in %d\n", sid, tid, item->id);
      
        ret = ivars->m_parent->removePortLink(item);
        if (ret != kIOReturnSuccess) {
            VSPLog(LOG_PREFIX, "unlinkPorts: parent removePortLink failed. code=%d\n", ret);
            set_ctlr_status(&response, ret, 0xfb000002);
        }
        else if (getLinkListHelper(&response) == kIOReturnSuccess) {
            VSPLog(LOG_PREFIX, "unlinkPorts finish.\n");
        }
    }

finish:
    return scheduleEvent(&response, arguments);
}

// --------------------------------------------------------------------
// Enable serial port parameter check on linkPorts()
//
kern_return_t VSP_IMPL_EX_METHOD("exEnableChecks", exEnableChecks, enableChecks)
kern_return_t VSPUserClient::enableChecks(void* reference, IOUserClientMethodArguments* arguments)
{
    kern_return_t ret;
    TVSPControllerData response = {};
    TVSPControllerData request = {};
    //uint8_t portId;

    VSPLog(LOG_PREFIX, "enableChecks called.\n");

    if ((ret = toRequest(arguments, &request)) != kIOReturnSuccess) {
        set_ctlr_status(&response, ret, 0xdd000000);
        goto finish;
    }

    VSP_INIT_RESPONSE(&request, MAGIC_CONTROL);

    if ((request.status.flags & MAGIC_CONTROL) != MAGIC_CONTROL) {
        set_ctlr_status(&response, kIOReturnInvalid, 0xee000000);
        goto finish;
    }

    // --
    // portId = request.parameter.link.source;

    VSPLog(LOG_PREFIX, "enableChecks finish.\n");

finish:
    return scheduleEvent(&response, arguments);
}

// --------------------------------------------------------------------
// Enable serial port protocol trace
//
kern_return_t VSP_IMPL_EX_METHOD("exEnableTrace", exEnableTrace, enableTrace)
kern_return_t VSPUserClient::enableTrace(void* reference, IOUserClientMethodArguments* arguments)
{
    kern_return_t ret;
    TVSPControllerData response = {};
    TVSPControllerData request = {};
    //uint8_t portId;
    
    VSPLog(LOG_PREFIX, "enableTrace called.\n");

    if ((ret = toRequest(arguments, &request)) != kIOReturnSuccess) {
        set_ctlr_status(&response, ret, 0xdd000000);
        goto finish;
    }

    VSP_INIT_RESPONSE(&request, MAGIC_CONTROL);

    if ((request.status.flags & MAGIC_CONTROL) != MAGIC_CONTROL) {
        set_ctlr_status(&response, kIOReturnInvalid, 0xee000000);
        goto finish;
    }

    // --
    //portId = request.parameter.link.source;
    
    VSPLog(LOG_PREFIX, "enableTrace finish.\n");

finish:
    return scheduleEvent(&response, arguments);
}
