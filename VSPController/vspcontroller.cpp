// ********************************************************************
// VSPController.cpp - VSPDriver user client controller
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
#include "vspcontroller.hpp"
#include "vspcontrollerpriv.hpp"
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOTypes.h>

#define BIT(x) (1 << x)

#ifndef VSP_DEBUG
#define VSP_DEBUG
#endif

namespace VSPClient {

VSPController::VSPController(const char* dextClassName)
{
    p = new VSPControllerPriv(dextClassName, this);
}

VSPController::~VSPController()
{
    delete p;
}

bool VSPController::ConnectDriver()
{
    return p->ConnectDriver();
}

bool VSPController::GetStatus()
{
    return p->GetStatus();
}

bool VSPController::IsConnected()
{
    return p->IsConnected();
}

bool VSPController::CreatePort(TVSPPortParameters* parameters)
{
    return p->CreatePort(parameters);
}

bool VSPController::RemovePort(const uint8_t id)
{
    return p->RemovePort(id);
}

bool VSPController::GetPortList()
{
    return p->GetPortList();
}

bool VSPController::GetLinkList()
{
    return p->GetLinkList();
}

bool VSPController::LinkPorts(const uint8_t source, const uint8_t target)
{
    return p->LinkPorts(source, target);
}

bool VSPController::UnlinkPorts(const uint8_t source, const uint8_t target)
{
    return p->UnlinkPorts(source, target);
}

bool VSPController::EnableChecks(const uint8_t port, const uint32_t flags)
{
    return p->EnableChecks(port, flags);
}

bool VSPController::EnableTrace(const uint8_t port, const uint32_t flags)
{
    return p->EnableTrace(port, flags);
}

bool VSPController::SetDextIdentifier(const char* name)
{
    return p->SetDextIdentifier(name);
}

bool VSPController::SendData(const TVSPControllerData& data)
{
    return p->SendData(data);
}

int VSPController::GetConnection()
{
    return p->m_drv;
}

const TVSPSystemError VSPController::GetSystemError(int error) const
{
    return {
       .system = err_get_system(error),
       .sub = err_get_sub(error),
       .code = err_get_code(error),
    };
}

const char* VSPController::DeviceName() const
{
    return p->DeviceName();
}

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
    if (ptr && length) {
        fprintf(stdout, "[%s] --------------------------\n{\n", ctx);
        for (uint32_t idx = 0; idx < length; ++idx) {
            fprintf(stdout, "\tptr[%02u] = %llu\n", idx, ptr[idx]);
        }
        fprintf(stdout, "}\n");
    }
}

static inline void PrintStruct(const char* ctx, const TVSPControllerData* ptr)
{
    if (ptr) {
        fprintf(stdout, "[%s] --------------------------\n{\n", ctx);
        fprintf(stdout, "\t.context = %u,\n", ptr->context);
        fprintf(stdout, "\t.command = %u,\n", ptr->command);
        fprintf(stdout, "\t.parameter.flags = 0x%llx,\n", ptr->parameter.flags);
        fprintf(stdout, "\t.ppl.sourceId = %u,\n", ptr->parameter.link.source);
        fprintf(stdout, "\t.ppl.targetId = %u,\n", ptr->parameter.link.target);
        fprintf(stdout, "\t.status.code = %u,\n", ptr->status.code);
        fprintf(stdout, "\t.status.flags = 0x%llx,\n", ptr->status.flags);
        fprintf(stdout, "}\n");
    }
}
#endif

static void DeviceAdded(void* refcon, io_iterator_t iterator)
{
    VSPControllerPriv* p = (VSPControllerPriv*) refcon;
    kern_return_t ret = kIOReturnNotFound;
    io_connect_t connection = IO_OBJECT_NULL;
    io_service_t device = IO_OBJECT_NULL;
    io_name_t deviceName = {};
    io_name_t devicePath = {};

    fprintf(stdout, "[VSPCTL] DeviceAdded(): ref=0x%llx\n", (uint64_t) refcon);

    // reset, emit disconnect if m_drv is not null
    p->SetConnection(IO_OBJECT_NULL);

    while ((device = IOIteratorNext(iterator)) != IO_OBJECT_NULL) {
        if ((ret = IORegistryEntryGetName(device, deviceName)) != kIOReturnSuccess) {
            p->ReportError(ret, "Get service registry name failed.");
        }
        if ((ret = IORegistryEntryGetPath(device, kIOServicePlane, devicePath)) != kIOReturnSuccess) {
            p->ReportError(ret, "Get service registry path failed.");
        }

        fprintf(stdout, "[VSPCTL] Open service: %s: %s\n", deviceName, devicePath);
        if (strlen(deviceName) > 0 && strlen(devicePath) > 0) {
            p->SetNameAndPath(deviceName, devicePath);
        }

        // Open a connection to this user client as a server
        // to that client, and store the instance in "service"
        if ((ret = IOServiceOpen(device, mach_task_self_, 0, &connection)) != kIOReturnSuccess) {
            if (ret == kIOReturnNotPermitted) {
                p->ReportError(ret, "Operation 'IOServiceOpen' not permitted.");
            }
            else {
                p->ReportError(ret, "Open service failed.");
            }
            IOObjectRelease(device);
            continue;
        }

        p->SetConnection(connection);

        IOObjectRelease(device);
        break;
    }
}

static void DeviceRemoved(void* refcon, io_iterator_t iterator)
{
    VSPControllerPriv* p = (VSPControllerPriv*) refcon;
    io_service_t device = IO_OBJECT_NULL;

    fprintf(stdout, "[VSPCTL] DeviceRemoved(): ref=0x%llx\n", (uint64_t) refcon);

    while ((device = IOIteratorNext(iterator)) != IO_OBJECT_NULL) {
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
    VSPControllerPriv* p = (VSPControllerPriv*) refcon;
    p->AsyncCallback(result, args, numArgs);
}

// -------------------------------------------------------------------

VSPControllerPriv::VSPControllerPriv(const char* dextClassName, VSPController* parent)
    : m_controller(parent)
    , m_machPort(0L)
    , m_runLoop(NULL)
    , m_runLoopSource(NULL)
    , m_deviceAddedIter(IO_OBJECT_NULL)
    , m_deviceRemovedIter(IO_OBJECT_NULL)
    , m_notificationPort(NULL)
    , m_drv(IO_OBJECT_NULL)
    , m_vspResponse(NULL)
{
    SetDextIdentifier(dextClassName);
}

VSPControllerPriv::~VSPControllerPriv()
{
    UserClientTeardown();
}

bool VSPControllerPriv::ConnectDriver()
{
    // already connected ?
    if (m_drv != 0)
        return true;

    // client setup up?
    if (!m_deviceAddedIter) {
        return UserClientSetup(this);
    }

    // find and connect extension
    DeviceAdded(this, m_deviceAddedIter);
    return m_drv != 0;
}

bool VSPControllerPriv::IsConnected()
{
    return (m_drv != IO_OBJECT_NULL);
}

bool VSPControllerPriv::GetStatus()
{
    TVSPControllerData input = {};
    input.context = vspContextPort;
    input.command = vspControlGetStatus;

    return DoAsyncCall(&input);
}

bool VSPControllerPriv::CreatePort(TVSPPortParameters* parameters)
{
    if (!parameters)
        return false;

    TVSPControllerData input = {};
    input.context = vspContextPort;
    input.command = vspControlCreatePort;
    input.parameter.link.source = 1;
    input.parameter.link.target = 1;

    if (sizeof(TVSPPortParameters) <= MAX_SERIAL_PORTS) {
        input.ports.count = sizeof(TVSPPortParameters);
        input.parameter.flags = 0xff01;
        memcpy(input.ports.list, parameters, input.ports.count);
    }

    return DoAsyncCall(&input);
}

bool VSPControllerPriv::RemovePort(const uint8_t id)
{
    TVSPControllerData input = {};
    input.context = vspContextPort;
    input.command = vspControlRemovePort;
    input.parameter.link.source = id;
    input.parameter.link.target = id;

    return DoAsyncCall(&input);
}

bool VSPControllerPriv::GetPortList()
{
    TVSPControllerData input = {};
    input.context = vspContextPort;
    input.command = vspControlGetPortList;

    return DoAsyncCall(&input);
}

bool VSPControllerPriv::GetLinkList()
{
    TVSPControllerData input = {};
    input.context = vspContextPort;
    input.command = vspControlGetLinkList;

    return DoAsyncCall(&input);
}

bool VSPControllerPriv::LinkPorts(const uint8_t source, const uint8_t target)
{
    TVSPControllerData input = {};
    input.context = vspContextPort;
    input.command = vspControlLinkPorts;
    input.parameter.link.source = source;
    input.parameter.link.target = target;

    return DoAsyncCall(&input);
}

bool VSPControllerPriv::UnlinkPorts(const uint8_t source, const uint8_t target)
{
    TVSPControllerData input = {};
    input.context = vspContextPort;
    input.command = vspControlUnlinkPorts;
    input.parameter.link.source = source;
    input.parameter.link.target = target;

    return DoAsyncCall(&input);
}

bool VSPControllerPriv::EnableChecks(const uint8_t port, const uint32_t flags)
{
    TVSPControllerData input = {};
    input.context = vspContextPort;
    input.command = vspControlEnableChecks;
    input.parameter.flags = flags;
    input.parameter.link.source = port;
    input.parameter.link.target = port;

    return DoAsyncCall(&input);
}

bool VSPControllerPriv::EnableTrace(const uint8_t port, const uint32_t flags)
{
    TVSPControllerData input = {};
    input.context = vspContextPort;
    input.command = vspControlEnableTrace;
    input.parameter.flags = flags;
    input.parameter.link.source = port;
    input.parameter.link.target = port;

    return DoAsyncCall(&input);
}

bool VSPControllerPriv::SendData(const TVSPControllerData& data)
{
    TVSPControllerData input = data;
    return DoAsyncCall(&input);
}

const char* VSPControllerPriv::DeviceName() const
{
    return m_deviceName;
}

const char* VSPControllerPriv::DevicePath() const
{
    return m_devicePath;
}

// -------------------------------------------------------------------
// If you don't know what value to use here, it should be identical to the
// IOUserClass value in your IOKitPersonalities. You can double check
// by searching with the `ioreg` command in your terminal. It will be of
// type "IOUserService" not "IOUserServer". The client Info.plist must
// contain:
// <key>com.apple.developer.driverkit.userclient-access</key>
// <array>
//     <string>VSPDriver</string>
// </array>
// <key>com.apple.private.driverkit.driver-access</key>
// <array>
//     <string>VSPDriver</string>
// </array>
//
inline bool VSPControllerPriv::SetDextIdentifier(const char* name)
{
    fprintf(stdout, "[VSPCTL] Using DEXT identifier: %s\n", name);

    if (name && strlen(name) < sizeof(TDextIdentifier)) {
        strncpy(m_dextClassName, name, sizeof(TDextIdentifier));
        return true;
    }

    return false;
}

void VSPControllerPriv::ReportError(IOReturn error, const char* message)
{
    const TVSPSystemError m_errorInfo = {
       .system = err_get_system(error),
       .sub = err_get_sub(error),
       .code = err_get_code(error),
    };

    fprintf(stderr, "[VSP Driver Error] ---------------------------------\n");
    fprintf(stderr, "%s\n", message);
    fprintf(stderr, "\tSystem...: 0x%04x\n", m_errorInfo.system);
    fprintf(stderr, "\tSubsystem: 0x%04x\n", m_errorInfo.code);
    fprintf(stderr, "\tCode.....: 0x%04x\n", m_errorInfo.code);

    m_controller->OnErrorOccured(m_errorInfo, message);
}

void VSPControllerPriv::SetNameAndPath(const char* name, const char* path)
{
    strncpy(m_deviceName, name, sizeof(m_deviceName) - 1);
    strncpy(m_devicePath, path, sizeof(m_devicePath) - 1);
}

bool VSPControllerPriv::UserClientSetup(void* refcon)
{
    kern_return_t ret = kIOReturnSuccess;

    fprintf(stderr, "[VSPCTL] UserClientSetup() ref=0x%llx\n", (uint64_t) refcon);

    m_runLoop = CFRunLoopGetCurrent();
    if (m_runLoop == NULL) {
        ReportError(kIOReturnError, "Failed to initialize run loop.");
        return false;
    }
    CFRetain(m_runLoop);

    if (__builtin_available(macOS 12.0, *)) {
        m_notificationPort = IONotificationPortCreate(kIOMainPortDefault);
    }
    else {
        return false;
    }
    if (m_notificationPort == NULL) {
        ReportError(kIOReturnError, "Failed to initialize motification port.");
        UserClientTeardown();
        return false;
    }

    m_machPort = IONotificationPortGetMachPort(m_notificationPort);
    if (m_notificationPort == 0) {
        ReportError(kIOReturnError, "Failed to initialize mach notification port.");
        UserClientTeardown();
        return false;
    }

    m_runLoopSource = IONotificationPortGetRunLoopSource(m_notificationPort);
    if (m_runLoopSource == NULL) {
        ReportError(kIOReturnError, "Failed to initialize run loop source.");
        UserClientTeardown();
        return false;
    }

    // Establish our notifications in the run loop, so we can get callbacks.
    CFRunLoopAddSource(m_runLoop, m_runLoopSource, kCFRunLoopDefaultMode);

    if (!strlen(m_dextClassName)) {
        ReportError(kIOReturnError, "DEXT Identifier is emptry, but required!");
        UserClientTeardown();
        return false;
    }

    fprintf(stdout, "[VSPCTL] Lookup DEXT identifier: %s\n", m_dextClassName);

    /// - Tag: SetUpMatchingNotification
    CFMutableDictionaryRef matchingDictionary = IOServiceNameMatching(m_dextClassName);
    if (matchingDictionary == NULL) {
        ReportError(kIOReturnError, "Failed to initialize matching dictionary.");
        UserClientTeardown();
        return false;
    }

    // retain for each callback
    matchingDictionary = (CFMutableDictionaryRef) CFRetain(matchingDictionary);
    matchingDictionary = (CFMutableDictionaryRef) CFRetain(matchingDictionary);

    // retain for callback
    ret = IOServiceAddMatchingNotification(
       m_notificationPort, kIOTerminatedNotification, matchingDictionary, DeviceRemoved, refcon, &m_deviceRemovedIter);
    if (ret != kIOReturnSuccess) {
        ReportError(ret, "Add termination notification failed.");
        UserClientTeardown();
        return false;
    }
    DeviceRemoved(refcon, m_deviceRemovedIter);

    ret = IOServiceAddMatchingNotification(
       m_notificationPort, kIOFirstMatchNotification, matchingDictionary, DeviceAdded, refcon, &m_deviceAddedIter);
    if (ret != kIOReturnSuccess) {
        ReportError(ret, "Add matching notification failed.");
        UserClientTeardown();
        return false;
    }
    DeviceAdded(refcon, m_deviceAddedIter);

    return (ret == kIOReturnSuccess) && (m_drv != 0);
}

inline void VSPControllerPriv::UserClientTeardown(void)
{
    if (m_vspResponse) {
        IOConnectUnmapMemory(m_drv, 0, mach_task_self(), (mach_vm_address_t) m_vspResponse);
        m_vspResponse = nullptr;
    }

    if (m_runLoopSource) {
        CFRunLoopRemoveSource(m_runLoop, m_runLoopSource, kCFRunLoopDefaultMode);
        m_runLoopSource = NULL;
    }

    if (m_notificationPort) {
        IONotificationPortDestroy(m_notificationPort);
        m_notificationPort = NULL;
    }

    if (m_runLoop) {
        CFRelease(m_runLoop);
        m_runLoop = NULL;
    }

    m_deviceAddedIter = IO_OBJECT_NULL;
    m_deviceRemovedIter = IO_OBJECT_NULL;
    m_drv = IO_OBJECT_NULL;
}

inline bool VSPControllerPriv::DoAsyncCall(TVSPControllerData* input)
{
    kern_return_t ret = kIOReturnSuccess;
    io_async_ref64_t ref = {};

    // set magic control
    input->status.flags = (MAGIC_CONTROL | BIT(input->command));

    // Establish our "AsyncCallback" function as the function that will be called
    // by our Dext when it calls its "AsyncCompletion" function.
    // We'll use kIOAsyncCalloutFuncIndex and kIOAsyncCalloutRefconIndex
    // to define the parameters for our async callback. This is your callback
    // function. Check the definition for more details.
    ref[kIOAsyncCalloutFuncIndex] = (io_user_reference_t) VSPClient::AsyncCallback;

    // Use this for context on the return. We'll pass the refcon so we can
    // talk back to the view model.
    ref[kIOAsyncCalloutRefconIndex] = (io_user_reference_t) this;

    // Instant response of the DEXT user client instance
    // Allocate response IOMemoryDescriptor at driver site
    // to response data above 128 bytes. This will filled,
    // by DEXT. We must unmap later!
    mach_vm_address_t address = 0;
    mach_vm_size_t size = 0;

    if (!m_vspResponse) {
        ret = IOConnectMapMemory64( //
           m_drv,                   // connection from IOServiceOpenv
           input->command,          // memoryType -> this is interpreted by VSP driver IOUserClient
           mach_task_self(),        // The task port for the task in which to create the mapping.
           &address,                // Returned address if success (kIOMapAnywhere)
           &size,                   // Returned memory size
           kIOMapAnywhere);         // see address
        if (ret != kIOReturnSuccess) {
            ReportError(ret, "Failed to get drivers mapped memory.");
            return false;
        }
        else if (!address || !size) {
            ReportError(ret, "Invalid mapped memory address or size.");
            return false;
        }
        else if (size < VSP_UCD_SIZE) {
            ReportError(ret, "Mapped memory size does not match requested size.");
            return false;
        }

        m_vspResponse = reinterpret_cast<TVSPControllerData*>(address);
    }

    // - do it --
    size_t resultSize = VSP_UCD_SIZE;
    ret = IOConnectCallAsyncStructMethod( //
       m_drv,                             // connection from IOServiceOpen
       input->command,                    // method selector
       m_machPort,                        // notification port
       ref,                               // call back structure
       kIOAsyncCalloutCount,              //
       input,                             // input parameters
       VSP_UCD_SIZE,                      // size of input parameters
       m_vspResponse,                     // shared kenel mapped response buffer
       &resultSize);                      // size of response, must be VSP_UCD_SIZE

    if (ret != kIOReturnSuccess) {
        ReportError(ret, "Driver async call failed.");
        IOConnectUnmapMemory(m_drv, input->command, mach_task_self(), address);
        m_vspResponse = nullptr;
        return false;
    }

    // we have fixed memory space.
    if (resultSize != VSP_UCD_SIZE) {
        ReportError(ret, "Driver responsed data size to small.");
        IOConnectUnmapMemory(m_drv, input->command, mach_task_self(), address);
        m_vspResponse = nullptr;
        return false;
    }

#ifdef VSP_DEBUG
    PrintStruct("doAsyncCall-Request", input);
    PrintStruct("doAsyncCall-Return", m_vspResponse);
#endif

    m_controller->OnDataReady((*m_vspResponse));
    return true;
}

void VSPControllerPriv::SetConnection(io_connect_t connection)
{
    if (connection != m_drv) {
        if (connection == 0) {
            m_controller->OnDisconnected();
            m_drv = 0;
            return;
        }

        m_drv = connection;
        m_controller->OnConnected();
    }
}

void VSPControllerPriv::AsyncCallback(IOReturn result, void** args, UInt32 numArgs)
{
    const int64_t* msg = (const int64_t*) args;

    // invalid driver message
    if (numArgs < 2 || !args) {
        ReportError(result, "Invalid driver message.");
        return;
    }

    if (result != kIOReturnSuccess) {
        ReportError(result, "Driver error occured.");
    }

#ifdef VSP_DEBUG
    PrintArray("AsyncCallback", msg, numArgs);
#endif

    // invalid driver signature
    if ((msg[0] & MAGIC_CONTROL) != MAGIC_CONTROL) {
        ReportError(result, "Invalid driver signature.");
        return;
    }

    if (m_vspResponse) {
        m_controller->OnIOUCCallback(result, m_vspResponse, VSP_UCD_SIZE);
    }
    else {
        ReportError(kIOReturnNotResponding, "[UC] No VSP async results.");
    }
}

} // END namspace VSPClient
