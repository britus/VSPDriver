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
#include <SerialDriverKit/SerialDriverKit.h>

// -- My
#include "VSPLogger.h"
#include "VSPUserClient.h"
#include "VSPController.h"
#include "VSPSerialPort.h"
#include "VSPDriver.h"

using namespace VSPController;

#define LOG_PREFIX "VSPUserClient"

#define VSP_METHOD(x) ((IOUserClientMethodFunction) &VSPUserClient::x)

#define VSP_CHECK_PARAM_RETURN(f, p) \
{ \
    if (p == nullptr) { \
        VSPErr(LOG_PREFIX, "%s: Invalid NULL argument '" f "' detected", __func__); \
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
#define VSP_IMPL_EX_METHOD(name, handler) \
VSPUserClient::name(OSObject* target, void* reference, IOUserClientMethodArguments* arguments) \
{ \
    VSP_HANDLER_CALL(target, handler) \
}

#ifdef DEBUG
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
        VSPLog(LOG_PREFIX, "\tPort item #%d: %d flags=%llx\n", i, //
               data->ports.list[i].id, data->ports.list[i].flags);
    }
    VSPLog(LOG_PREFIX, "Data.links.count......: %d\n", data->links.count);
    for (uint8_t i = 0; i < data->links.count && i < MAX_SERIAL_PORTS; i++) {
        VSPLog(LOG_PREFIX, "\tPort Link #%d: %d <-> %d\n", //
               ((uint8_t)((data->links.list[i] >> 16) & 0xffL)),
               ((uint8_t)((data->links.list[i] >> 8) & 0xffL)),
               ((uint8_t)((data->links.list[i]) & 0xffL)));
    }
}
#define VSP_DUMP_DATA(data) dump_ctrl_data(data)
#else
#define VSP_DUMP_DATA(data)
#endif

#define kVSPUserClientQueueId "VSPUserClient.uc.disp.queue"

struct VSPUserClient_IVars {
    IOService* m_provider = nullptr;
    VSPDriver* m_parent = nullptr;
    IODispatchQueue* m_evQueue = nullptr;
    IOTimerDispatchSource* m_evSource = nullptr;
    OSAction* m_evAction = nullptr;
    OSAction* m_cbAction = nullptr;
    TVSPControllerData m_response = {};
};

// Define all possible commands with its parameters and callback entry points.
const IOUserClientMethodDispatch uc_methods[vspLastCommand] = {
    [vspControlPingPong] =
    {
        .function = VSP_METHOD(exRestoreSession),
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
    [vspControlShutdown] =
    {
        .function = VSP_METHOD(exShutdown),
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
        VSPErr(LOG_PREFIX, "init (super) falsed. result=%d\n", result);
        goto finish;
    }
    
    // Create instance state resource
    ivars = IONewZero(VSPUserClient_IVars, 1);
    if (!ivars) {
        VSPErr(LOG_PREFIX, "Unable to allocate driver data.\n");
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
        VSPErr(LOG_PREFIX, "Start: Private driver instance is NULL\n");
        return kIOReturnInvalid;
    }
    
    // Start service instance (Apple style super call)
    ret = Start(provider, SUPERDISPATCH);
    if (ret != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "Start(super): failed. code=%x\n", ret);
        return ret;
    }
    
    ret = IODispatchQueue::Create(kVSPUserClientQueueId, 0, 0, &ivars->m_evQueue);
    if (ret != kIOReturnSuccess)
    {
        VSPErr(LOG_PREFIX, "Start: IODispatchQueue::Create failed with error: 0x%08x.", ret);
        return ret;
    }
    
    ret = IOTimerDispatchSource::Create(ivars->m_evQueue, &ivars->m_evSource);
    if (ret != kIOReturnSuccess)
    {
        VSPErr(LOG_PREFIX, "Start: IOTimerDispatchSource::Create failed with error: 0x%08x.", ret);
        goto error_exit;
    }
    
    ret = CreateActionAsyncCallback(VSP_UCD_SIZE, &ivars->m_evAction);
    if (ret != kIOReturnSuccess)
    {
        VSPErr(LOG_PREFIX, "Start: CreateActionAsyncCallback failed with error: 0x%08x.", ret);
        goto error_exit;
    }
    
    ret = ivars->m_evSource->SetHandler(ivars->m_evAction);
    if (ret != kIOReturnSuccess)
    {
        VSPErr(LOG_PREFIX, "Start: Failed to assign action to handler with error: 0x%08x.", ret);
        goto error_exit;
    }
    
    // --
    // Register driver instance to IOReg
    if ((ret = RegisterService()) != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "Start: RegisterService failed. code=%x\n", ret);
        goto error_exit;
    }

    VSPLog(LOG_PREFIX, "User client successfully started.\n");
    return kIOReturnSuccess;
    
error_exit:
    OSSafeReleaseNULL(ivars->m_evAction);
    OSSafeReleaseNULL(ivars->m_evSource);
    OSSafeReleaseNULL(ivars->m_evQueue);

    return ret;
}

// --------------------------------------------------------------------
// Stop_Impl(IOService* provider)
//
kern_return_t IMPL(VSPUserClient, Stop)
{
    kern_return_t ret;
    
    VSPLog(LOG_PREFIX, "Stop called.\n");
    
    OSSafeReleaseNULL(ivars->m_evAction);
    OSSafeReleaseNULL(ivars->m_evSource);
    OSSafeReleaseNULL(ivars->m_evQueue);
    OSSafeReleaseNULL(ivars->m_cbAction);

    // service instance (Apple style super call)
    if ((ret= Stop(provider, SUPERDISPATCH)) != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "Stop (super) failed. code=%x\n", ret);
    } else {
        VSPLog(LOG_PREFIX, "User client successfully removed.\n");
    }
    
    return ret;
}

// --------------------------------------------------------------------
// CopyClientMemoryForType_Impl(uint64_t type, uint64_t *options, IOMemoryDescriptor **memory)
// Allocate space for UC. The user space must be call IOConnectUnmapMemory
//
kern_return_t IMPL(VSPUserClient, CopyClientMemoryForType)
{
    IOBufferMemoryDescriptor* md;
    kern_return_t ret = kIOReturnSuccess;

    VSPLog(LOG_PREFIX, "CopyClientMemoryForType called. type=%llu optionsptr=0x%llx\n",
           type, (uint64_t)options);

    if (!memory) {
        VSPErr(LOG_PREFIX, "CopyClientMemoryForType Invalid argument.");
        return kIOReturnBadArgument;
    }
    
    (*memory) = nullptr;

    if (type >= vspControlPingPong && type < vspLastCommand)
    {
        // share only one memory buffer
        ret = IOBufferMemoryDescriptor::Create(kIOMemoryDirectionInOut, VSP_UCD_SIZE, 0, &md);
        if (ret == kIOReturnSuccess) {
            if (md) {
                (*memory) = md; // returned with refcount 1
            }
            else {
                ret = kIOReturnNoSpace;
            }
        }
    }
    else {
        ret = this->CopyClientMemoryForType(type, options, memory, SUPERDISPATCH);
    }
    
    if (ret != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "CopyClientMemoryForType failed: 0x%x", ret);
    }
    else {
        VSPLog(LOG_PREFIX, "CopyClientMemoryForType complete. code=%d\n", ret);
    }
    
    return ret;
}

// --------------------------------------------------------------------
// AsyncCallback_IMPL(...)
// Event raised by IOTimerDispatchSource::TimerOccurred event
//
void IMPL(VSPUserClient, AsyncCallback)
{
    VSPLog(LOG_PREFIX, "AsyncCallback called.\n");
    
    if (!action) { // Kernel timer event action object
        VSPErr(LOG_PREFIX, "AsyncCallback: Event action NULL pointer!");
        return;
    }
    if (!ivars->m_cbAction) { // UC completion action object
        VSPErr(LOG_PREFIX, "AsyncCallback: Skip, client callback action NULL pointer!");
        return;
    }

    // Get back our data previously stored in OSAction.
    TVSPControllerData* data = (TVSPControllerData*) action->GetReference();
    
    // Debug !!
    VSP_DUMP_DATA(data);

    // Async callback message to user client. This is limited
    // by IOUserClientAsyncReferenceArray declared as
    // typedef uint64_t[16] !!! (128 bytes)
    uint64_t* message = IONewZero(uint64_t, 5);
    message[0] = MAGIC_CONTROL;
    message[1] = VSP_UCD_SIZE;
    message[2] = data->context;
    message[3] = data->command;
    message[4] = 0x001f;

    // dispatch message back to user client
    AsyncCompletion(ivars->m_cbAction, data->status.code, message, 5);
    IOSafeDeleteNULL(message, uint64_t, 5);

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
    
    VSPLog(LOG_PREFIX, "ExternalMethod called. selector=%llu arguments=0x%llx dispatch=0x%llx target=0x%llx reference=0x%llx\n",
           selector,
           (uint64_t)arguments,
           (uint64_t)dispatch,
           (uint64_t)target,
           (uint64_t)reference);

    if (selector >= vspLastCommand) {
        VSPErr(LOG_PREFIX, "Invalid method selector detected, skip.");
        return kIOReturnBadArgument;
    }

    VSP_CHECK_PARAM_RETURN("arguments", arguments);
   
    VSPLog(LOG_PREFIX,
           "args: scalarInputCount=%llu scalarOutputCount=%llu "
           "structureOutputMaximumSize=%llu "
           "structureInput=%p structureInputDescriptor=%p structureOutput=%p completion=%p",
           (uint64_t)arguments->scalarInputCount,
           (uint64_t)arguments->scalarOutputCount,
           (uint64_t)arguments->structureOutputMaximumSize,
           arguments->structureInput,
           arguments->structureInputDescriptor,
           arguments->structureOutput,
           arguments->completion);

    if (!arguments->completion) {
        arguments->completion = ivars->m_evAction;
    }
    
    // Set method dispatch descriptor
    dispatch  = &uc_methods[selector];
    target    = this;
    reference = ivars;

    // This will call the functions as defined in the IOUserClientMethodDispatch.
    ret = super::ExternalMethod(selector, arguments, dispatch, target, reference);
    if (ret != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "ExternalMethod dispatch failed. code=%x", ret);
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
// Prepares async response
//
kern_return_t VSPUserClient::prepareResponse(void* reference, IOUserClientMethodArguments* arguments)
{
    const TVSPControllerData* response = reinterpret_cast<TVSPControllerData*>(reference);
    IOMemoryMap* outputMap = nullptr;
    uint8_t* outputPtr;
    void* evData;
    uint64_t length;
    IOReturn ret;
    
    VSPLog(LOG_PREFIX, "prepareResponse called.\n");

    if (!response) {
        VSPErr(LOG_PREFIX, "prepareResponse: Invalid argument 'reference' detected.\n");
        return kIOReturnBadArgument;
    }

    if (!arguments) {
        VSPErr(LOG_PREFIX, "prepareResponse: Invalid argument 'arguments' detected.\n");
        return kIOReturnBadArgument;
    }
 
    VSP_DUMP_DATA(response);
    
    // Memory is not passed from the caller into the dext.
    // The dext needs to create its own OSData to hold this
    // information in order to pass it back to the caller.
    if (arguments->structureOutputDescriptor != nullptr)
    {
        VSPLog(LOG_PREFIX, "prepareResponse: using arguments->structureOutputDescriptor\n");

        // need at least minimum of VSP_UCD_SIZE as output size
        if (VSP_UCD_SIZE > arguments->structureOutputMaximumSize)
        {
            VSPErr(LOG_PREFIX,
                   "prepareResponse: Required output size of %lu"
                   "is larger than the given maximum UC size of %llu. Failing.",
                   VSP_UCD_SIZE, arguments->structureOutputMaximumSize);
            return kIOReturnNoSpace;
        }

        ret = arguments->structureOutputDescriptor->CreateMapping(0, 0, 0, 0, 0, &outputMap);
        if (ret != kIOReturnSuccess || !outputMap) {
            VSPErr(LOG_PREFIX, "prepareResponse: UC output CreateMapping failed. code=%x\n", ret);
        }
        else {
            outputPtr = (uint8_t*) outputMap->GetAddress();
            length    = outputMap->GetLength();
            
            VSPLog(LOG_PREFIX, "prepareResponse: Share result to UC. length: %lld addr: 0x%llx\n",
                   length, (uint64_t) outputPtr);
            
            // Copy the data from response record over
            memcpy(outputPtr, response, (length < VSP_UCD_SIZE
                                         ? length
                                         : VSP_UCD_SIZE));
            
            // and then fill the rest with zeroes.
            if ((length = (length - VSP_UCD_SIZE)) > 0) {
                memset(outputPtr + VSP_UCD_SIZE, 0, length);
            }
            
            OSSafeReleaseNULL(outputMap);
        }
    }
    else {
        VSPLog(LOG_PREFIX, "prepareResponse: create OSData for arguments->structureOutput\n");
        arguments->structureOutput = OSData::withBytes(response, VSP_UCD_SIZE);
    }
    
    // Save the completion for later. If not saved, then it
    // might be freed before the asychronous return.
    if (!arguments->completion) {
        ivars->m_cbAction = nullptr;
        goto finish;
    } else {
        ivars->m_cbAction = arguments->completion;
        // Retain action memory for later work.
        ivars->m_cbAction->retain();
    }
    
    // Fill event data with response. It action GetReference() returns NULL
    // if the action object doesn't belong to the current process.
    if ((evData = ivars->m_evAction->GetReference()) != nullptr) {
        memcpy(evData, response, VSP_UCD_SIZE);
    }
    else {
        VSPErr(LOG_PREFIX, "prepareResponse: Event action data does not belong current process!\n");
    }
    
finish:
    VSPLog(LOG_PREFIX, "prepareResponse finish.\n");
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// Dispatch async event
kern_return_t VSPUserClient::scheduleEvent(void* reference, IOUserClientMethodArguments* arguments)
{
    TVSPControllerData* response = (TVSPControllerData*) reference;
    IOReturn ret;
 
    VSPLog(LOG_PREFIX, "scheduleEvent called.\n");
    
    if ((ret = prepareResponse(response, arguments)) != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "scheduleEvent: Preprare response failed. code=%x\n", ret);
        set_ctlr_status(response, ret, 0xea000001);
    }
    
    if (ivars->m_cbAction) {
        uint64_t       currentTime = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);
        currentTime               += 1000000;
        const uint64_t leeway      = 1000000000;
        return ivars->m_evSource->WakeAtTime(kIOTimerClockMonotonicRaw, currentTime, leeway);
    }
    
    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// Get user's request data from method arguments
//
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
            VSPErr(LOG_PREFIX, "toRequest: Failed to create mapping for descriptor with error: 0x%08x", ret);
            return kIOReturnBadArgument;
        }
        size = inputMap->GetLength();
        buffer = (void*) inputMap->GetAddress();
        memcpy(request, buffer, (size < VSP_UCD_SIZE ? size : VSP_UCD_SIZE));
        OSSafeReleaseNULL(inputMap);
    }
    else
    {
        VSPErr(LOG_PREFIX, "toRequest: Both structureInput and structureInputDescriptor were null.");
        return kIOReturnBadArgument;
    }
    
    VSP_DUMP_DATA(request);
   
    if ((request->status.flags & MAGIC_CONTROL) != MAGIC_CONTROL) {
        return kIOReturnUnsupported;
    }

    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// Prepare default response object based on request object
//
inline static IOReturn toResponse( //
            struct VSPUserClient_IVars* ivars,  //
            const TVSPControllerData* request,  //
                TVSPControllerData* response)
{
    if (!request) {
        return kIOReturnBadArgument;
    }
    if (!response) {
        return kIOReturnBadArgument;
    }
    
    response->context = vspContextResult;
    response->command = request->command;
    response->status.code = kIOReturnSuccess;
    response->status.flags= MAGIC_CONTROL;
    
    /* global flags */
    response->traceFlags = ivars->m_parent->traceFlags();
    response->checkFlags = ivars->m_parent->checkFlags();

    return kIOReturnSuccess;
}

// --------------------------------------------------------------------
// MARK: Static External Handlers
// --------------------------------------------------------------------

// --------------------------------------------------------------------
// Return status of VSPDriver instance
//
kern_return_t VSP_IMPL_EX_METHOD(exRestoreSession, restoreSession)
kern_return_t VSPUserClient::restoreSession(void* reference, IOUserClientMethodArguments* arguments)
{
    const size_t psize = sizeof(TVSPPortItem);
    const size_t lsize = sizeof(TVSPLinkItem);
    TVSPControllerData response = {};
    TVSPControllerData request = {};
    TVSPPortItem port = {};
    TVSPLinkItem link = {};
    kern_return_t ret;
    uint8_t sid, tid, lid, pfh, pfl;
    uint64_t flg; /*, mk1, mk2;*/
    
    VSPLog(LOG_PREFIX, "restoreSession called.\n");

    if ((ret = toRequest(arguments, &request)) != kIOReturnSuccess) {
        set_ctlr_status(&response, ret, 0xee00ee00);
        goto finish;
    }
    if ((ret = toResponse(ivars, &request, &response)) != kIOReturnSuccess) {
        set_ctlr_status(&response, ret, 0xee00ee01);
        goto finish;
    }

    pfh = ((request.parameter.flags >> 8) & 0xffL);
    pfl = (request.parameter.flags & 0xffL);
    if ((pfh & 0xff) != 0xff || (pfl & 0x08) != 0x08) {
        goto finish;
    }
    
    if (request.ports.count) {
        for (uint8_t i = 0; i < request.ports.count && i < MAX_SERIAL_PORTS; i++) {
            sid = request.ports.list[i].id;
            flg = request.ports.list[i].flags;
            if (ivars->m_parent->getPortById(sid, &port, psize) == kIOReturnNotFound) {
                ivars->m_parent->createPort(nullptr, flg, 0);
            }
        }
    }

    if (request.links.count) {
        for (uint8_t i = 0; i < request.links.count && i < MAX_PORT_LINKS; i++) {
            lid = (request.links.list[i] >> 16) & 0xffL;
            sid = (request.links.list[i] >> 8) & 0xffL;
            tid = (request.links.list[i] & 0xffL);
            if (ivars->m_parent->getPortLinkById(lid, &link, lsize) == kIOReturnNotFound) {
                ivars->m_parent->createPortLink(sid, tid, &link, lsize);
            }
        }
    }

    VSPLog(LOG_PREFIX, "restoreSession complete.\n");

finish:
    return getStatus(ivars, arguments);
}

// --------------------------------------------------------------------
// Return status of VSPDriver instance
//
kern_return_t VSP_IMPL_EX_METHOD(exGetStatus, getStatus)
kern_return_t VSPUserClient::getStatus(void* reference, IOUserClientMethodArguments* arguments)
{
    TVSPControllerData response = {};
    TVSPControllerData request = {};
    kern_return_t ret;

    VSPLog(LOG_PREFIX, "getStatus called.\n");
    
    if ((ret = toRequest(arguments, &request)) != kIOReturnSuccess) {
        set_ctlr_status(&response, ret, 0xee00ee00);
        goto finish;
    }
    if ((ret = toResponse(ivars, &request, &response)) != kIOReturnSuccess) {
        set_ctlr_status(&response, ret, 0xee00ee01);
        goto finish;
    }

    if ((ret = getPortListHelper(&response)) != kIOReturnSuccess) {
        set_ctlr_status(&response, ret, 0xfe000001);
        goto finish;
    }
    else if (response.ports.count) {
        response.parameter.flags |= BIT(1);
    }
 
    if ((ret = getLinkListHelper(&response)) != kIOReturnSuccess) {
        set_ctlr_status(&response, ret, 0xfe000002);
        goto finish;
    }
    else if (response.links.count) {
        response.parameter.flags |= BIT(2);
    }
    
    VSPLog(LOG_PREFIX, "getStatus finish.\n");
    
finish:
    return scheduleEvent(&response, arguments);
}

// --------------------------------------------------------------------
// Create new serial port instance
//
kern_return_t VSP_IMPL_EX_METHOD(exCreatePort, createPort)
kern_return_t VSPUserClient::createPort(void* reference, IOUserClientMethodArguments* arguments)
{
    TVSPControllerData response = {};
    TVSPControllerData request = {};
    TVSPPortParameters params = {115200, 8, 2, PD_RS232_PARITY_NONE, 0};
    kern_return_t ret;
    
    VSPLog(LOG_PREFIX, "createPort called.\n");

    if ((ret = toRequest(arguments, &request)) != kIOReturnSuccess) {
        set_ctlr_status(&response, ret, 0xee00ee00);
        goto finish;
    }
    if ((ret = toResponse(ivars, &request, &response)) != kIOReturnSuccess) {
        set_ctlr_status(&response, ret, 0xee00ee01);
        goto finish;
    }

    // extract magic from parameter flags
    if (request.ports.count == sizeof(TVSPPortParameters)) {
        VSPLog(LOG_PREFIX, "createPort with parameters.\n");
        if (request.parameter.flags & 0xff01) {
            memcpy(&params, request.ports.list, sizeof(TVSPPortParameters));
        }
    }
    
    if ((ret = ivars->m_parent->createPort(&params, 0, sizeof(TVSPPortParameters))) != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "createPort: Parent createPort failed. code=%x\n", ret);
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
kern_return_t VSP_IMPL_EX_METHOD(exRemovePort, removePort)
kern_return_t VSPUserClient::removePort(void* reference, IOUserClientMethodArguments* arguments)
{
    kern_return_t ret;
    TVSPControllerData response = {};
    TVSPControllerData request = {};
    uint8_t portId = 0;

    VSPLog(LOG_PREFIX, "removePort called.\n");

    if ((ret = toRequest(arguments, &request)) != kIOReturnSuccess) {
        set_ctlr_status(&response, ret, 0xee00ee00);
        goto finish;
    }
    if ((ret = toResponse(ivars, &request, &response)) != kIOReturnSuccess) {
        set_ctlr_status(&response, ret, 0xee00ee01);
        goto finish;
    }

    portId = request.parameter.link.source;
    if ((ret = ivars->m_parent->removePort(portId))!= kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "removePort: Parent removePort failed. code=%x\n", ret);
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
kern_return_t VSP_IMPL_EX_METHOD(exGetPortList, getPortList)
kern_return_t VSPUserClient::getPortList(void* reference, IOUserClientMethodArguments* arguments)
{
    kern_return_t ret;
    TVSPControllerData response = {};
    TVSPControllerData request = {};

    VSPLog(LOG_PREFIX, "getPortList called.\n");

    if ((ret = toRequest(arguments, &request)) != kIOReturnSuccess) {
        set_ctlr_status(&response, ret, 0xee00ee00);
        goto finish;
    }
    if ((ret = toResponse(ivars, &request, &response)) != kIOReturnSuccess) {
        set_ctlr_status(&response, ret, 0xee00ee01);
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
    uint8_t count = 0;
    
    if ((ret = ivars->m_parent->getPortCount(&count)) != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "getPortListHelper: parent getPortCount failed. code=%x\n", ret);
        set_ctlr_status(response, ret, 0xfc000001);
    }
    else if (count == 0) {
        /* be quiet to caller */
        VSPErr(LOG_PREFIX, "getPortListHelper: No serial ports available.\n");
        response->ports.count = 0;
    }
    else if ((ret = ivars->m_parent->getPortList(response->ports.list, count)) != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "getPortListHelper: parent getPortList failed. code=%x\n", ret);
        set_ctlr_status(response, ret, 0xfc000002);
    }
    else {
        response->ports.count = count;
    }

    return response->status.code;
}

// --------------------------------------------------------------------
// Return active port link list
//
kern_return_t VSP_IMPL_EX_METHOD(exGetLinkList, getLinkList)
kern_return_t VSPUserClient::getLinkList(void* reference, IOUserClientMethodArguments* arguments)
{
    kern_return_t ret;
    TVSPControllerData response = {};
    TVSPControllerData request = {};

    VSPLog(LOG_PREFIX, "getLinkList called.\n");

    if ((ret = toRequest(arguments, &request)) != kIOReturnSuccess) {
        set_ctlr_status(&response, ret, 0xee00ee00);
        goto finish;
    }
    if ((ret = toResponse(ivars, &request, &response)) != kIOReturnSuccess) {
        set_ctlr_status(&response, ret, 0xee00ee01);
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
    uint8_t count = 0;

    if ((ret = ivars->m_parent->getPortLinkCount(&count)) != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "getLinkListHelper: parent getPortLinkCount failed. code=%x\n", ret);
        set_ctlr_status(response, ret, 0xd0000001);
    }
    else if (count == 0) {
        /* be quiet to caller */
        VSPLog(LOG_PREFIX, "getLinkListHelper: No port links available.\n");
        response->links.count = 0;
    }
    else if ((ret = ivars->m_parent->getPortLinkList(response->links.list, count)) != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "getLinkListHelper: parent getPortLinkList failed. code=%x\n", ret);
        set_ctlr_status(response, ret, 0xd0000002);
    }
    else {
        response->links.count = count;
    }
    
    return response->status.code;
}

// --------------------------------------------------------------------
// Link two serial ports together
//
kern_return_t VSP_IMPL_EX_METHOD(exLinkPorts, linkPorts)
kern_return_t VSPUserClient::linkPorts(void* reference, IOUserClientMethodArguments* arguments)
{
    kern_return_t ret;
    TVSPControllerData response = {};
    TVSPControllerData request = {};
    TVSPLinkItem link = {};
    uint8_t sid, tid;

    VSPLog(LOG_PREFIX, "linkPorts called.\n");

    if ((ret = toRequest(arguments, &request)) != kIOReturnSuccess) {
        set_ctlr_status(&response, ret, 0xee00ee00);
        goto finish;
    }
    if ((ret = toResponse(ivars, &request, &response)) != kIOReturnSuccess) {
        set_ctlr_status(&response, ret, 0xee00ee01);
        goto finish;
    }

    sid = request.parameter.link.source;
    tid = request.parameter.link.target;
   
    ret = ivars->m_parent->createPortLink(sid, tid, &link, sizeof(TVSPLinkItem));
    if (ret != kIOReturnSuccess) {
        VSPErr(LOG_PREFIX, "linkPorts: parent createPortLink failed. code=%x\n", ret);
        set_ctlr_status(&response, ret, 0xfa000001);
    }
    else if (link.id) {
        if (getLinkListHelper(&response) == kIOReturnSuccess) {
            VSPLog(LOG_PREFIX, "linkPorts finish.\n");
        }
    }
    else {
        VSPErr(LOG_PREFIX, "linkPorts: Got invalid link Id %d. code=%x\n",
               link.id, ret);
        set_ctlr_status(&response, ret, 0xfa000002);
    }


finish:
    return scheduleEvent(&response, arguments);
}

// --------------------------------------------------------------------
// Unlink prior linked ports
//
kern_return_t VSP_IMPL_EX_METHOD(exUnlinkPorts, unlinkPorts)
kern_return_t VSPUserClient::unlinkPorts(void* reference, IOUserClientMethodArguments* arguments)
{
    kern_return_t ret;
    TVSPControllerData response = {};
    TVSPControllerData request = {};
    TVSPLinkItem link = {};
    uint8_t sid, tid;

    VSPLog(LOG_PREFIX, "unlinkPorts called.\n");
    
    if ((ret = toRequest(arguments, &request)) != kIOReturnSuccess) {
        set_ctlr_status(&response, ret, 0xee00ee00);
        goto finish;
    }
    if ((ret = toResponse(ivars, &request, &response)) != kIOReturnSuccess) {
        set_ctlr_status(&response, ret, 0xee00ee01);
        goto finish;
    }

    sid = request.parameter.link.source;
    tid = request.parameter.link.target;
    
    ret = ivars->m_parent->getPortLinkByPorts(sid, tid, &link, sizeof(TVSPLinkItem));
    if (ret != kIOReturnSuccess) {
        set_ctlr_status(&response, ret, 0xfb000001);
    }
    else if (link.id) {
        VSPLog(LOG_PREFIX, "unlinkPorts: remove src=%d tgt=%d in %d\n", sid, tid, link.id);
      
        ret = ivars->m_parent->removePortLink(link.id);
        if (ret != kIOReturnSuccess) {
            VSPErr(LOG_PREFIX, "unlinkPorts: parent removePortLink failed. code=%x\n", ret);
            set_ctlr_status(&response, ret, 0xfb000002);
        }
        else if (getLinkListHelper(&response) == kIOReturnSuccess) {
            VSPLog(LOG_PREFIX, "unlinkPorts finish.\n");
        }
    }
    else {
        VSPErr(LOG_PREFIX, "unlinkPorts: Got invalid linkId failed. code=%x\n", ret);
        set_ctlr_status(&response, ret, 0xfa000002);
    }

finish:
    return scheduleEvent(&response, arguments);
}

// --------------------------------------------------------------------
// Enable serial port parameter check on linkPorts()
//
kern_return_t VSP_IMPL_EX_METHOD(exEnableChecks, enableChecks)
kern_return_t VSPUserClient::enableChecks(void* reference, IOUserClientMethodArguments* arguments)
{
    kern_return_t ret;
    TVSPControllerData response = {};
    TVSPControllerData request = {};
    //TVSPPortItem item = {};
    //uint8_t portId;

    VSPLog(LOG_PREFIX, "enableChecks called.\n");

    if ((ret = toRequest(arguments, &request)) != kIOReturnSuccess) {
        set_ctlr_status(&response, ret, 0xee00ee00);
        goto error;
    }
    if ((ret = toResponse(ivars, &request, &response)) != kIOReturnSuccess) {
        set_ctlr_status(&response, ret, 0xee00ee01);
        goto error;
    }

    ivars->m_parent->setCheckFlags(request.checkFlags);
    ivars->m_parent->setTraceFlags(request.traceFlags);

    return getPortList(reference, arguments);

error:
    return scheduleEvent(&response, arguments);
}

// --------------------------------------------------------------------
// deprecated, wrap to enableChecks() function
//
kern_return_t VSP_IMPL_EX_METHOD(exEnableTrace, enableTrace)
kern_return_t VSPUserClient::enableTrace(void* reference, IOUserClientMethodArguments* arguments)
{
    return enableChecks(reference, arguments);
}

// --------------------------------------------------------------------
// shutdown driver by crash
//
kern_return_t VSP_IMPL_EX_METHOD(exShutdown, shutdown)
kern_return_t VSPUserClient::shutdown(void* reference, IOUserClientMethodArguments* arguments)
{
    // KILL driver - no other way!
    __builtin_trap();
    
    return kIOReturnAborted;
}
