// ********************************************************************
// VSPController.cpp - VSPDriver user client controller
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
#include <iostream>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOTypes.h>
#include <IOKit/IOKitLib.h>
#include "VSPController.hpp"
#include "VSPControllerPriv.hpp"

#define BIT(x) (1 << x)
#define VSP_DEBUG

namespace VSPClient {

VSPController::VSPController()
{
    p = new VSPControllerPriv(this);
}

VSPController::~VSPController()
{
    delete p;
}

// -------------------------------------------------------------------
//
//
bool VSPController::ConnectDriver()
{
    return p->ConnectDriver();
}

// -------------------------------------------------------------------
//
//
bool VSPController::GetStatus()
{
    return p->GetStatus();
}

// -------------------------------------------------------------------
//
//
bool VSPController::IsConnected()
{
    return p->IsConnected();
}

// -------------------------------------------------------------------
//
//
bool VSPController::CreatePort(TVSPPortParameters* parameters)
{
    return p->CreatePort(parameters);
}

// -------------------------------------------------------------------
//
//
bool VSPController::RemovePort(const uint8_t id)
{
    return p->RemovePort(id);
}

// -------------------------------------------------------------------
//
//
bool VSPController::GetPortList()
{
    return p->GetPortList();
}

// -------------------------------------------------------------------
//
//
bool VSPController::GetLinkList()
{
    return p->GetLinkList();
}

// -------------------------------------------------------------------
//
//
bool VSPController::LinkPorts(const uint8_t source, const uint8_t target)
{
    return p->LinkPorts(source, target);
}

// -------------------------------------------------------------------
//
//
bool VSPController::UnlinkPorts(const uint8_t source, const uint8_t target)
{
    return p->UnlinkPorts(source, target);
}

// -------------------------------------------------------------------
//
//
bool VSPController::EnableChecks(const uint8_t port)
{
    return p->EnableChecks(port);
}

// -------------------------------------------------------------------
//
//
bool VSPController::EnableTrace(const uint8_t port)
{
    return p->EnableTrace(port);
}

// -------------------------------------------------------------------
//
//
int VSPController::GetConnection()
{
    return p->m_connection;
}

// -------------------------------------------------------------------
//
//
const char* VSPController::DeviceName() const
{
    return p->DeviceName();
}

// -------------------------------------------------------------------
//
//
const char* VSPController::DevicePath() const
{
    return p->DevicePath();
}

// -------------------------------------------------------------------
// MARK: Private Section
// -------------------------------------------------------------------

#ifdef VSP_DEBUG
static inline void PrintArray(const char* ctx, const int64_t* ptr, const uint32_t length)
{
    printf("[%s] --------------------------\n{\n", ctx);
    for (uint32_t idx = 0; idx < length; ++idx)
    {
        printf("ptr[%02u] = %llu\n", idx, ptr[idx]);
    }
    printf("}\n");
}
static inline void PrintStruct(const char* ctx, const TVSPControllerData* ptr)
{
    printf("[%s] --------------------------\n{\n", ctx);
    printf("\t.context = %u,\n", ptr->context);
    printf("\t.command = %u,\n", ptr->command);
    printf("\t.parameter.flags = 0x%llx,\n", ptr->parameter.flags);
    printf("\t.ppl.sourceId = %u,\n", ptr->parameter.portLink.sourceId);
    printf("\t.ppl.targetId = %u,\n", ptr->parameter.portLink.targetId);
    printf("\t.status.code = %u,\n", ptr->status.code);
    printf("\t.status.flags = 0x%llx,\n", ptr->status.flags);
    printf("}\n");
}
#endif

// -------------------------------------------------------------------
//
//
static inline void PrintErrorDetails(kern_return_t ret)
{
    fprintf(stderr, "\tVSP Error:\n");
    fprintf(stderr, "\tSystem...: 0x%02x\n", err_get_system(ret));
    fprintf(stderr, "\tSubsystem: 0x%03x\n", err_get_sub(ret));
    fprintf(stderr, "\tCode.....: 0x%04x\n", err_get_code(ret));
}

VSPControllerPriv::VSPControllerPriv(VSPController* parent)
    : m_machNotificationPort(NULL)
    , m_runLoop(NULL)
    , m_runLoopSource(NULL)
    , m_deviceAddedIter(IO_OBJECT_NULL)
    , m_deviceRemovedIter(IO_OBJECT_NULL)
    , m_notificationPort(NULL)
    , m_connection(NULL)
    , m_controller(parent)
{
}

VSPControllerPriv::~VSPControllerPriv()
{
    UserClientTeardown();
}

// -------------------------------------------------------------------
//
//
bool VSPControllerPriv::ConnectDriver()
{
    UserClientSetup(this);
    return IsConnected();
}

// -------------------------------------------------------------------
//
//
bool VSPControllerPriv::IsConnected()
{
    return (m_connection != 0L);
}

// -------------------------------------------------------------------
//
//
bool VSPControllerPriv::GetStatus()
{
    TVSPControllerData input = {};
    input.context = vspContextPort;
    input.command = vspControlGetStatus;
    
    return DoAsyncCall(&input);
}

// -------------------------------------------------------------------
//
//
bool VSPControllerPriv::CreatePort(TVSPPortParameters* parameters)
{
    if (!parameters)
        return false;
    
    TVSPControllerData input = {};
    input.context = vspContextPort;
    input.command = vspControlCreatePort;
    input.ports.count = 8;
    input.ports.list[0] = ((parameters->baudRate >> 24) & 0x00000ff);
    input.ports.list[1] = ((parameters->baudRate >> 16) & 0x00000ff);
    input.ports.list[2] = ((parameters->baudRate >> 8) & 0x00000ff);
    input.ports.list[3] = (parameters->baudRate & 0x00000ff);
    input.ports.list[4] = parameters->dataBits;
    input.ports.list[5] = parameters->stopBits + 1;
    input.ports.list[6] = parameters->parity;
    input.ports.list[7] = parameters->flowCtrl;
    return DoAsyncCall(&input);
}

// -------------------------------------------------------------------
//
//
bool VSPControllerPriv::RemovePort(const uint8_t id)
{
    TVSPControllerData input = {};
    input.context = vspContextPort;
    input.command = vspControlRemovePort;
    input.parameter.portLink.sourceId = id;
    input.parameter.portLink.targetId = id;
    
    return DoAsyncCall(&input);
}

// -------------------------------------------------------------------
//
//
bool VSPControllerPriv::GetPortList()
{
    TVSPControllerData input = {};
    input.context = vspContextPort;
    input.command = vspControlGetPortList;
    
    return DoAsyncCall(&input);
}

// -------------------------------------------------------------------
//
//
bool VSPControllerPriv::GetLinkList()
{
    TVSPControllerData input = {};
    input.context = vspContextPort;
    input.command = vspControlGetLinkList;
    
    return DoAsyncCall(&input);
}

// -------------------------------------------------------------------
//
//
bool VSPControllerPriv::LinkPorts(const uint8_t source, const uint8_t target)
{
    TVSPControllerData input = {};
    input.context = vspContextPort;
    input.command = vspControlLinkPorts;
    input.parameter.portLink.sourceId = source;
    input.parameter.portLink.targetId = target;
    
    return DoAsyncCall(&input);
}

// -------------------------------------------------------------------
//
//
bool VSPControllerPriv::UnlinkPorts(const uint8_t source, const uint8_t target)
{
    TVSPControllerData input = {};
    input.context = vspContextPort;
    input.command = vspControlUnlinkPorts;
    input.parameter.portLink.sourceId = source;
    input.parameter.portLink.targetId = target;
    
    return DoAsyncCall(&input);
}

// -------------------------------------------------------------------
//
//
bool VSPControllerPriv::EnableChecks(const uint8_t port)
{
    TVSPControllerData input = {};
    input.context = vspContextPort;
    input.command = vspControlEnableChecks;
    input.parameter.portLink.sourceId = port;
    input.parameter.portLink.targetId = port;
    
    return DoAsyncCall(&input);
}

// -------------------------------------------------------------------
//
//
bool VSPControllerPriv::EnableTrace(const uint8_t port)
{
    TVSPControllerData input = {};
    input.context = vspContextPort;
    input.command = vspControlEnableTrace;
    input.parameter.portLink.sourceId = port;
    input.parameter.portLink.targetId = port;
    
    return DoAsyncCall(&input);
}

// -------------------------------------------------------------------
//
//
const char* VSPControllerPriv::DeviceName() const
{
    return m_deviceName;
}

// -------------------------------------------------------------------
//
//
const char* VSPControllerPriv::DevicePath() const
{
    return m_devicePath;
}

// -------------------------------------------------------------------
// MARK: Private Asynchronous Events
// -------------------------------------------------------------------

// -------------------------------------------------------------------
//
//
static void DeviceAdded(void* refcon, io_iterator_t iterator)
{
    VSPControllerPriv* p = (VSPControllerPriv*) refcon;
    kern_return_t ret = kIOReturnNotFound;
    io_connect_t connection = IO_OBJECT_NULL;
    io_service_t device = IO_OBJECT_NULL;
    bool clientFound = false;
    io_name_t deviceName = {};
    io_name_t devicePath = {};
    
    while ((device = IOIteratorNext(iterator)) != IO_OBJECT_NULL)
    {
        //attemptedToMatchDevice = true;
        if ((ret = IORegistryEntryGetName(device, deviceName)) != kIOReturnSuccess) {
            p->ReportError(ret, "Get service registry name failed.");
        }
        if ((ret = IORegistryEntryGetPath(device, kIOServicePlane, devicePath)) != kIOReturnSuccess) {
            p->ReportError(ret, "Get service registry path failed.");
        }
        if (strlen(deviceName) > 0 && strlen(devicePath) > 0) {
            p->SetNameAndPath(deviceName, devicePath);
        }
        
        // Open a connection to this user client as a server
        // to that client, and store the instance in "service"
        if ((ret = IOServiceOpen(device, mach_task_self_, 0, &connection)) != kIOReturnSuccess) {
            if (ret == kIOReturnNotPermitted) {
                p->ReportError(ret, "Operation 'IOServiceOpen' not permitted.");
            } else {
                p->ReportError(ret, "Open service failed.");
            }
            IOObjectRelease(device);
            continue;
        }
        
        IOObjectRelease(device);
        
        //-> SwiftDeviceAdded(refcon, connection);
        p->SetConnection(connection);
        
        clientFound = true;
        ret = kIOReturnSuccess;
    }
    
    if (!clientFound) {
        p->ReportError(kIOReturnNotFound, "Unable to find VSPDriver extensions.");
    }
}

// -------------------------------------------------------------------
//
//
static void DeviceRemoved(void* refcon, io_iterator_t iterator)
{
    VSPControllerPriv* p = (VSPControllerPriv*) refcon;
    io_service_t device = IO_OBJECT_NULL;
    
    while ((device = IOIteratorNext(iterator)) != IO_OBJECT_NULL)
    {
        IOObjectRelease(device);
        p->SetConnection(0L);
    }
}

// -------------------------------------------------------------------
// For more detail on this callback format, view the format of:
// IOAsyncCallback, IOAsyncCallback0, IOAsyncCallback1, IOAsyncCallback2
// Note that the variant of IOAsyncCallback called is based on the
// number of arguments being returned
// 0  - IOAsyncCallback0
// 1  - IOAsyncCallback1
// 2  - IOAsyncCallback2
// 3+ - IOAsyncCallback
// This is an example of the "IOAsyncCallback" format.
// refcon will be the value you placed in asyncRef[kIOAsyncCalloutRefconIndex]
static void AsyncCallback(void* refcon, IOReturn result, void** args, UInt32 numArgs)
{
    //-> App API callback SwiftAsyncCallback(refcon, result, args, numArgs);
    VSPControllerPriv* p = (VSPControllerPriv*) refcon;
    p->AsyncCallback(result, args, numArgs);
}

// -------------------------------------------------------------------
// MARK: Private API Implementation
// -------------------------------------------------------------------

// -------------------------------------------------------------------
//
//
void VSPControllerPriv::ReportError(IOReturn error, const char* message)
{
    PrintErrorDetails(error);

    m_controller->OnErrorOccured(error, message);
}

// -------------------------------------------------------------------
//
//
void VSPControllerPriv::SetNameAndPath(const char* name, const char* path)
{
    strncpy(m_deviceName, name, sizeof(m_deviceName)-1);
    strncpy(m_devicePath, path, sizeof(m_devicePath)-1);
}

// -------------------------------------------------------------------
//
//
bool VSPControllerPriv::UserClientSetup(void* refcon)
{
    kern_return_t ret = kIOReturnSuccess;
    
    m_runLoop = CFRunLoopGetCurrent();
    if (m_runLoop == NULL)
    {
        ReportError(kIOReturnError, "Failed to initialize run loop.");
        return false;
    }
    CFRetain(m_runLoop);
    
    m_notificationPort = IONotificationPortCreate(kIOMainPortDefault);
    if (m_notificationPort == NULL)
    {
        ReportError(kIOReturnError, "Failed to initialize motification port.");
        UserClientTeardown();
        return false;
    }
    
    m_machNotificationPort = IONotificationPortGetMachPort(m_notificationPort);
    if (m_machNotificationPort == 0)
    {
        ReportError(kIOReturnError, "Failed to initialize mach notification port.");
        UserClientTeardown();
        return false;
    }
    
    m_runLoopSource = IONotificationPortGetRunLoopSource(m_notificationPort);
    if (m_runLoopSource == NULL)
    {
        ReportError(kIOReturnError, "Failed to initialize run loop source.");
        return false;
    }
    
    // Establish our notifications in the run loop, so we can get callbacks.
    CFRunLoopAddSource(m_runLoop, m_runLoopSource, kCFRunLoopDefaultMode);
    
    /// - Tag: SetUpMatchingNotification
    CFMutableDictionaryRef matchingDictionary = IOServiceNameMatching(dextIdentifier);
    if (matchingDictionary == NULL)
    {
        ReportError(kIOReturnError, "Failed to initialize matching dictionary.");
        UserClientTeardown();
        return false;
    }
    matchingDictionary = (CFMutableDictionaryRef)CFRetain(matchingDictionary);
    matchingDictionary = (CFMutableDictionaryRef)CFRetain(matchingDictionary);
    
    ret = IOServiceAddMatchingNotification(m_notificationPort,
                                           kIOFirstMatchNotification,
                                           matchingDictionary,
                                           DeviceAdded, refcon,
                                           &m_deviceAddedIter);
    if (ret != kIOReturnSuccess)
    {
        ReportError(ret, "Add matching notification failed.");
        UserClientTeardown();
        return false;
    }
    DeviceAdded(refcon, m_deviceAddedIter);
    
    ret = IOServiceAddMatchingNotification(m_notificationPort,
                                           kIOTerminatedNotification,
                                           matchingDictionary,
                                           DeviceRemoved, refcon,
                                           &m_deviceRemovedIter);
    if (ret != kIOReturnSuccess)
    {
        ReportError(ret, "Add termination notification failed.");
        UserClientTeardown();
        return false;
    }
    DeviceRemoved(refcon, m_deviceRemovedIter);
    
    return true;
}

// -------------------------------------------------------------------
//
//
void VSPControllerPriv::UserClientTeardown(void)
{
    if (m_runLoopSource)
    {
        CFRunLoopRemoveSource(m_runLoop, m_runLoopSource, kCFRunLoopDefaultMode);
        m_runLoopSource = NULL;
    }
    
    if (m_notificationPort)
    {
        IONotificationPortDestroy(m_notificationPort);
        m_notificationPort = NULL;
        m_machNotificationPort = 0;
    }
    
    if (m_runLoop)
    {
        CFRelease(m_runLoop);
        m_runLoop = NULL;
    }
    
    m_deviceAddedIter   = IO_OBJECT_NULL;
    m_deviceRemovedIter = IO_OBJECT_NULL;
    m_connection        = IO_OBJECT_NULL;
}

// -------------------------------------------------------------------
//
//
inline bool VSPControllerPriv::DoAsyncCall(TVSPControllerData* input)
{
    kern_return_t ret = kIOReturnSuccess;
    io_async_ref64_t asyncRef = {};
    
    // set magic control
    input->status.flags = (MAGIC_CONTROL | BIT(input->command));

    // Establish our "AsyncCallback" function as the function that will be called
    // by our Dext when it calls its "AsyncCompletion" function.
    // We'll use kIOAsyncCalloutFuncIndex and kIOAsyncCalloutRefconIndex
    // to define the parameters for our async callback. This is your callback
    // function. Check the definition for more details.
    asyncRef[kIOAsyncCalloutFuncIndex]   = (io_user_reference_t) VSPClient::AsyncCallback;
    
    // Use this for context on the return. We'll pass the refcon so we can
    // talk back to the view model.
    asyncRef[kIOAsyncCalloutRefconIndex] = (io_user_reference_t) this;
    
    // Instant response of the DEXT user client instance
    size_t resultSize = VSP_UCD_SIZE;
    TVSPControllerData result = { };
    
    ret = IOConnectCallAsyncStructMethod(m_connection,
                                         input->command,
                                         m_machNotificationPort,
                                         asyncRef,
                                         kIOAsyncCalloutCount,
                                         input, VSP_UCD_SIZE, &result, &resultSize);
    if (ret != kIOReturnSuccess) {
        ReportError(ret, "Driver async call failed.");
        return false;
    }
    
#ifdef VSP_DEBUG
    PrintStruct("doAsyncCall-Request", input);
    PrintStruct("doAsyncCall-Return", &result);
#endif

    m_vspResult = result;
    m_controller->OnDataReady(&m_vspResult);
    return true;
}

// -------------------------------------------------------------------
//
//
void VSPControllerPriv::SetConnection(io_connect_t connection)
{
    if (connection) {
        m_connection = connection;
        m_controller->OnConnected();
    } else {
        m_connection = NULL;
        m_controller->OnDisconnected();
    }
}

// -------------------------------------------------------------------
//
//
void VSPControllerPriv::AsyncCallback(IOReturn result, void** args, UInt32 numArgs)
{
#ifdef VSP_DEBUG
    PrintArray("AsyncCallback", (const int64_t*) args, numArgs);
#endif

    m_controller->OnIOUCCallback(result, args, numArgs);
}

} // END namspace VSPClient
