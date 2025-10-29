// ********************************************************************
// VSPClient native OS Bridge
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOTypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* VSP controller data and class */
#include "VSPController.h"

#define BIT(x) (1 << x)

#ifndef VSP_DEBUG
#define VSP_DEBUG
#endif

extern "C" { // for Swift ->
extern void ConvertDataFromCPP(const void *pInput, size_t size);
extern void SendLogMessage(const char* buffer, size_t size);
}

// -------------------------------------------------------------------
// MARK: Private Section
// -------------------------------------------------------------------

#ifdef VSP_DEBUG
static inline void printArray(const char *ctx, const int64_t *ptr, const uint32_t length)
{
    if (ptr && length) {
        // Use a buffer to collect output before converting to NSString
        char buffer[4096];
        size_t offset = 0;
        
        // Helper function to safely append to buffer
        #define BUFFER_APPEND(fmt, ...) \
            do { \
                int len = snprintf(buffer + offset, sizeof(buffer) - offset, fmt, ##__VA_ARGS__); \
                if (len > 0 && len < (int)(sizeof(buffer) - offset)) { \
                    offset += len; \
                } else if (len >= (int)sizeof(buffer)) { \
                    buffer[sizeof(buffer)-1] = '\0'; \
                } \
            } while(0)
        
        BUFFER_APPEND("[%s] --------------------------\n{\n", ctx);
        for (uint32_t idx = 0; idx < length; ++idx) {
            BUFFER_APPEND("\tptr[%02u] = %llu\n", idx, (unsigned long long)ptr[idx]);
        }
        BUFFER_APPEND("}\n");
        
        // call Swift callback
        SendLogMessage(buffer, offset);

        #undef BUFFER_APPEND
    }
}

static inline void printStruct(const char *ctx, const CVSPDriverData *ptr)
{
    if (ptr) {
        // Use a buffer to collect output before converting to NSString
        char buffer[4096];
        size_t offset = 0;
        
        // Helper function to safely append to buffer
        #define BUFFER_APPEND(fmt, ...) \
            do { \
                int len = snprintf(buffer + offset, sizeof(buffer) - offset, fmt, ##__VA_ARGS__); \
                if (len > 0 && len < (int)(sizeof(buffer) - offset)) { \
                    offset += len; \
                } else if (len >= (int)sizeof(buffer)) { \
                    buffer[sizeof(buffer)-1] = '\0'; \
                } \
            } while(0)
        
        BUFFER_APPEND("[%s] --------------------------\n{\n", ctx);
        BUFFER_APPEND("\t.context = %u,\n", ptr->context);
        BUFFER_APPEND("\t.command = %u,\n", ptr->command);
        BUFFER_APPEND("\t.traces = 0x%llx,\n", ptr->traceFlags);
        BUFFER_APPEND("\t.checks = 0x%llx,\n", ptr->checkFlags);
        BUFFER_APPEND("\t.parameter.flags = 0x%llx,\n", ptr->parameter.flags);
        if ((ptr->parameter.flags & 0xff01) != ptr->parameter.flags) {
            BUFFER_APPEND("\t.ppl.sourceId = %u,\n", ptr->parameter.link.source);
            BUFFER_APPEND("\t.ppl.targetId = %u,\n", ptr->parameter.link.target);
            BUFFER_APPEND("\t.status.code = 0x%x,\n", ptr->status.code);
            BUFFER_APPEND("\t.status.flags = 0x%llx,\n", ptr->status.flags);
            if (ptr->ports.count) {
                for (uint8_t i = 0; i < ptr->ports.count; i++) {
                    BUFFER_APPEND("\t.port = %d flags = 0x%llx,\n", //
                            ptr->ports.list[i].id,
                            ptr->ports.list[i].flags);
                }
            }
            if (ptr->links.count) {
                for (uint8_t i = 0; i < ptr->links.count; i++) {
                    BUFFER_APPEND("\t.link = %d link = 0x%llx,\n", //
                            i, ptr->links.list[i]);
                }
            }
        } else if (ptr->ports.count == sizeof(CVSPPortParameters)) {
            CVSPPortParameters* p = (CVSPPortParameters*)(ptr->ports.list);
            BUFFER_APPEND("\t.baudRate = %u,\n", p->baudRate);
            BUFFER_APPEND("\t.dataBits = %u,\n", p->dataBits);
            BUFFER_APPEND("\t.stopBits = %u,\n", p->stopBits);
            BUFFER_APPEND("\t.parity   = %u,\n", p->parity);
            BUFFER_APPEND("\t.flowCtrl = %u,\n", p->flowCtrl);
        }
        
        BUFFER_APPEND("}\n");
        
        // call Swift callback
        SendLogMessage(buffer, offset);

        #undef BUFFER_APPEND
    }
}
#endif // VSP_DEBUG

/* -------------------------------------------------------------------
 * DriverKit DEXT IOUserClient connect / disconnect callbacks
 * ------------------------------------------------------------------- */

static void DeviceAdded(void *refcon, io_iterator_t iterator)
{
    VSPController *p = (VSPController *) refcon;
    if (!(p = (VSPController *) refcon)) {
        fprintf(stderr, "[VSPCTL] DeviceAdded(): refcon is NULL\n");
        return;
    }
    
    kern_return_t ret = kIOReturnNotFound;
    io_connect_t connection = IO_OBJECT_NULL;
    io_service_t device = IO_OBJECT_NULL;
    io_name_t deviceName = {};
    io_name_t devicePath = {};

    fprintf(stdout, "[VSPCTL] DeviceAdded(): ref=0x%llx\n", (uint64_t) refcon);

    // reset, emit disconnect if m_drv is not null
    p->setConnection(IO_OBJECT_NULL);

    while ((device = IOIteratorNext(iterator)) != IO_OBJECT_NULL) {
        if ((ret = IORegistryEntryGetName(device, deviceName)) != kIOReturnSuccess) {
            p->reportError(ret, "Get service registry name failed.");
        }
        if ((ret = IORegistryEntryGetPath(device, kIOServicePlane, devicePath)) != kIOReturnSuccess) {
            p->reportError(ret, "Get service registry path failed.");
        }

        fprintf(stdout, "[VSPCTL] Open service: %s: %s\n", deviceName, devicePath);
        if (strlen(deviceName) > 0 && strlen(devicePath) > 0) {
            p->setNameAndPath(deviceName, devicePath);
        }

        // Open a connection to this user client as a server
        // to that client, and store the instance in "service"
        if ((ret = IOServiceOpen(device, mach_task_self_, 0, &connection)) != kIOReturnSuccess) {
            if (ret == kIOReturnNotPermitted) {
                p->reportError(ret, "Operation 'IOServiceOpen' not permitted.");
            } else {
                p->reportError(ret, "Open service failed.");
            }
            IOObjectRelease(device);
            continue;
        }

        p->setConnection(connection);

        IOObjectRelease(device);
        break;
    }
}

static void DeviceRemoved(void *refcon, io_iterator_t iterator)
{
    VSPController *p;
    if ((p = (VSPController *) refcon)) {
        io_service_t device = IO_OBJECT_NULL;
        
        fprintf(stdout, "[VSPCTL] DeviceRemoved(): ref=0x%llx\n", (uint64_t) refcon);
        
        while ((device = IOIteratorNext(iterator)) != IO_OBJECT_NULL) {
            IOObjectRelease(device);
            p->setConnection(0L);
        }
    }
}

/* -------------------------------------------------------------------
 * DriverKit DEXT IOUserClient::ExternalMethod async callback
 * ------------------------------------------------------------------- */
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
static void AsyncCallback(void *refcon, IOReturn result, void **args, UInt32 numArgs)
{
    VSPController *p;
    if ((p = (VSPController *) refcon)) {
        p->asyncCallback(result, args, numArgs);
    }
}

/* -------------------------------------------------------------------
 * DriverKit DEXT access controller
 * ------------------------------------------------------------------- */

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

static mach_port_t           m_machPort;
static CFRunLoopRef          m_runLoop = NULL;
static CFRunLoopSourceRef    m_runLoopSource = NULL;
static io_iterator_t         m_deviceAddedIter = IO_OBJECT_NULL;
static io_iterator_t         m_deviceRemovedIter = IO_OBJECT_NULL;
static IONotificationPortRef m_notificationPort = NULL;
static io_connect_t          m_connection = IO_OBJECT_NULL;
static io_name_t             m_deviceName;
static io_name_t             m_devicePath;

VSPController::VSPController(const char *dextClassName)
    : m_vspResponse(NULL)
{
    setDextClassName(dextClassName);
}

VSPController::~VSPController()
{
    userClientTeardown();
}

const CVSPSystemError VSPController::systemError(int error) const
{
    return {
        .system = err_get_system(error),
        .sub = err_get_sub(error),
        .code = err_get_code(error),
    };
}

void VSPController::asyncCallback(IOReturn result, void **args, UInt32 numArgs)
{
    const int64_t *msg = (const int64_t *) args;

    // invalid driver message
    if (numArgs < 2 || !args) {
        reportError(result, "Invalid driver message.");
        return;
    }

    if (result != kIOReturnSuccess) {
        reportError(result, "Driver error occured.");
    }

#ifdef VSP_DEBUG
    printArray("asyncCallback", msg, numArgs);
#endif

    // invalid driver signature
    if ((msg[0] & MAGIC_CONTROL) != MAGIC_CONTROL) {
        reportError(result, "Invalid driver signature.");
        return;
    }

    if (!m_vspResponse) {
        reportError(kIOReturnNotResponding, "[UC] No VSP async results.");
        return;
    }
}

bool VSPController::ConnectDriver()
{
    // already connected ?
    if (m_connection != 0)
        return true;

    // client setup up?
    if (!m_deviceAddedIter) {
        return userClientSetup(this);
    }

    // find and connect extension
    DeviceAdded(this, m_deviceAddedIter);
    return m_connection != 0;
}

bool VSPController::IsConnected()
{
    return (m_connection != IO_OBJECT_NULL);
}

bool VSPController::GetStatus()
{
    CVSPDriverData input = {};
    input.context = vspContextPort;
    input.command = vspControlGetStatus;

    return asyncCall(&input);
}

bool VSPController::CreatePort(CVSPPortParameters *parameters)
{
    if (!parameters)
        return false;
    
    CVSPDriverData input = {};
    input.context = vspContextPort;
    input.command = vspControlCreatePort;
    input.parameter.link.source = 1;
    input.parameter.link.target = 1;

    if (sizeof(CVSPPortParameters) <= MAX_SERIAL_PORTS) {
        input.ports.count = sizeof(CVSPPortParameters);
        input.parameter.flags = 0xff01;
        memcpy(input.ports.list, parameters, input.ports.count);
    }

    return asyncCall(&input);
}

bool VSPController::RemovePort(const uint8_t id)
{
    CVSPDriverData input = {};
    input.context = vspContextPort;
    input.command = vspControlRemovePort;
    input.parameter.link.source = id;
    input.parameter.link.target = id;

    return asyncCall(&input);
}

bool VSPController::GetPortList()
{
    CVSPDriverData input = {};
    input.context = vspContextPort;
    input.command = vspControlGetPortList;

    return asyncCall(&input);
}

bool VSPController::GetLinkList()
{
    CVSPDriverData input = {};
    input.context = vspContextPort;
    input.command = vspControlGetLinkList;

    return asyncCall(&input);
}

bool VSPController::LinkPorts(const uint8_t source, const uint8_t target)
{
    CVSPDriverData input = {};
    input.context = vspContextPort;
    input.command = vspControlLinkPorts;
    input.parameter.link.source = source;
    input.parameter.link.target = target;

    return asyncCall(&input);
}

bool VSPController::UnlinkPorts(const uint8_t source, const uint8_t target)
{
    CVSPDriverData input = {};
    input.context = vspContextPort;
    input.command = vspControlUnlinkPorts;
    input.parameter.link.source = source;
    input.parameter.link.target = target;

    return asyncCall(&input);
}

bool VSPController::EnableChecksAndTrace(const uint8_t port, const uint64_t checkFlags, const uint64_t traceFlags)
{
    CVSPDriverData input = {};
    input.context = vspContextPort;
    input.command = vspControlEnableChecks;
    input.checkFlags = checkFlags;
    input.traceFlags = traceFlags;
    input.parameter.flags = 0;//(checkFlags | traceFlags);
    input.parameter.link.source = port;
    input.parameter.link.target = port;

    return asyncCall(&input);
}

bool VSPController::SendData(const CVSPDriverData &data)
{
    CVSPDriverData input = data;
    return asyncCall(&input);
}

const char *VSPController::deviceName() const
{
    return m_deviceName;
}

const char *VSPController::devicePath() const
{
    return m_devicePath;
}

inline bool VSPController::setDextClassName(const char *name)
{
    if (name && strlen(name) < sizeof(TDextIdentifier)) {
        fprintf(stdout, "[VSPCTL] Using DEXT class name: %s\n", name);
        strncpy(m_dextClassName, name, sizeof(TDextIdentifier));
        return true;
    }

    return false;
}

void VSPController::reportError(IOReturn error, const char *message)
{
    m_errorInfo = {
        .oscode = error,
        .system = err_get_system(error),
        .sub    = err_get_sub(error),
        .code   = err_get_code(error),
    };

    fprintf(stderr, "[VSP Driver Error] ---------------------------------\n");
    fprintf(stderr, "%s\n", message);
    fprintf(stderr, "\tOS Code..: 0x%08x\n", m_errorInfo.oscode);
    fprintf(stderr, "\tSystem...: 0x%04x\n", m_errorInfo.system);
    fprintf(stderr, "\tSubsystem: 0x%04x\n", m_errorInfo.code);
    fprintf(stderr, "\tCode.....: 0x%04x\n", m_errorInfo.code);

    //m_controller->OnErrorOccured(m_errorInfo, message);
}

void VSPController::setNameAndPath(const char *name, const char *path)
{
    strncpy(m_deviceName, name, sizeof(m_deviceName) - 1);
    strncpy(m_devicePath, path, sizeof(m_devicePath) - 1);
}

bool VSPController::userClientSetup(void *refcon)
{
    kern_return_t ret = kIOReturnSuccess;

    // sane check DEXT class name!
    if (!strlen(m_dextClassName)) {
        reportError(kIOReturnError, "DEXT Identifier is emptry, but required!");
        userClientTeardown();
        return false;
    }

    fprintf(stderr, "[VSPCTL] UserClientSetup() ref=0x%llx className=%s\n", (uint64_t) refcon, m_dextClassName);

    m_runLoop = CFRunLoopGetCurrent();
    if (m_runLoop == NULL) {
        reportError(kIOReturnError, "Failed to initialize run loop.");
        return false;
    }
    CFRetain(m_runLoop);

    if (__builtin_available(macOS 12.0, *)) {
        m_notificationPort = IONotificationPortCreate(kIOMainPortDefault);
    } else {
        return false;
    }
    if (m_notificationPort == NULL) {
        reportError(kIOReturnError, "Failed to initialize motification port.");
        userClientTeardown();
        return false;
    }

    m_machPort = IONotificationPortGetMachPort(m_notificationPort);
    if (m_notificationPort == 0) {
        reportError(kIOReturnError, "Failed to initialize mach notification port.");
        userClientTeardown();
        return false;
    }

    m_runLoopSource = IONotificationPortGetRunLoopSource(m_notificationPort);
    if (m_runLoopSource == NULL) {
        reportError(kIOReturnError, "Failed to initialize run loop source.");
        userClientTeardown();
        return false;
    }

    fprintf(stdout, "[VSPCTL] Lookup DEXT identifier: %s\n", m_dextClassName);

    // Establish our notifications in the run loop, so we can get callbacks.
    CFRunLoopAddSource(m_runLoop, m_runLoopSource, kCFRunLoopDefaultMode);

    /// - Tag: SetUpMatchingNotification
    CFMutableDictionaryRef matchingDictionary = IOServiceNameMatching(m_dextClassName);
    if (matchingDictionary == NULL) {
        reportError(kIOReturnError, "Failed to initialize matching dictionary.");
        userClientTeardown();
        return false;
    }

    // retain for each callback
    matchingDictionary = (CFMutableDictionaryRef) CFRetain(matchingDictionary);
    matchingDictionary = (CFMutableDictionaryRef) CFRetain(matchingDictionary);

    // retain for callback
    ret = IOServiceAddMatchingNotification(m_notificationPort, kIOTerminatedNotification, matchingDictionary, DeviceRemoved, refcon, &m_deviceRemovedIter);
    if (ret != kIOReturnSuccess) {
        reportError(ret, "Add termination notification failed.");
        userClientTeardown();
        return false;
    }
    DeviceRemoved(refcon, m_deviceRemovedIter);

    ret = IOServiceAddMatchingNotification(m_notificationPort, kIOFirstMatchNotification, matchingDictionary, DeviceAdded, refcon, &m_deviceAddedIter);
    if (ret != kIOReturnSuccess) {
        reportError(ret, "Add matching notification failed.");
        userClientTeardown();
        return false;
    }
    DeviceAdded(refcon, m_deviceAddedIter);

    return (ret == kIOReturnSuccess) && (m_connection != 0);
}

inline void VSPController::userClientTeardown(void)
{
    if (m_vspResponse) {
        IOConnectUnmapMemory(m_connection, 0, mach_task_self(), (mach_vm_address_t) m_vspResponse);
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
    m_connection = IO_OBJECT_NULL;
}

inline bool VSPController::asyncCall(CVSPDriverData *input)
{
    kern_return_t ret = kIOReturnSuccess;
    io_async_ref64_t descriptor = {};

#ifdef VSP_DEBUG
    printStruct("asyncCall-Request", input);
#endif

    // set magic control
    input->status.flags = (MAGIC_CONTROL | BIT(input->command));

    // Establish our "AsyncCallback" function as the function that will be called
    // by our Dext when it calls its "AsyncCompletion" function.
    // We'll use kIOAsyncCalloutFuncIndex and kIOAsyncCalloutRefconIndex
    // to define the parameters for our async callback. This is your callback
    // function. Check the definition for more details.
    descriptor[kIOAsyncCalloutFuncIndex] = (io_user_reference_t) AsyncCallback;

    // Use this for context on the return. We'll pass the refcon so we can
    // talk back to the view model.
    descriptor[kIOAsyncCalloutRefconIndex] = (io_user_reference_t) this;

    if (!m_vspResponse) {
        // Instant response of the DEXT user client instance
        // Allocate response IOMemoryDescriptor at driver site
        // to response data above 128 byte limit. This will filled,
        // by DEXT. We must unmap later!
        mach_vm_address_t address = 0;
        mach_vm_size_t size = 0;

        ret = IOConnectMapMemory64( //
            m_connection,                  // connection from IOServiceOpen
            input->command,         // memoryType -> this is interpreted by VSP driver IOUserClient
            mach_task_self(),       // The task port for the task in which to create the mapping.
            &address,               // Returned address if success (kIOMapAnywhere)
            &size,                  // Returned memory size
            kIOMapAnywhere);        // see address
        if (ret != kIOReturnSuccess) {
            reportError(ret, "Failed to get drivers mapped memory.");
            return false;
        } else if (!address) {
            reportError(ret, "Invalid mapped memory address.");
            return false;
        } else if (size != VSP_UCD_SIZE) {
            reportError(ret, "Mapped memory size does not match requested size.");
            return false;
        }

        m_vspResponse = reinterpret_cast<CVSPDriverData *>(address);
    }

    // - do it --
    size_t resultSize = VSP_UCD_SIZE;
    ret = IOConnectCallAsyncStructMethod( //
        m_connection,                            // Connection from IOServiceOpen
        input->command,                   // Method selector
        m_machPort,                       // Notification port
        descriptor,                       // Call back structure
        kIOAsyncCalloutCount,             //
        input,                            // Input parameters
        VSP_UCD_SIZE,                     // Size of input parameters
        m_vspResponse,                    // Kernel mapped response buffer
        &resultSize);                     // Size of response, must be VSP_UCD_SIZE

    if (ret != kIOReturnSuccess) {
        reportError(ret, "Driver async call failed.");
        IOConnectUnmapMemory(m_connection, input->command, mach_task_self(), (mach_vm_address_t) m_vspResponse);
        m_vspResponse = nullptr;
        return false;
    }

    // we have fixed memory space.
    if (resultSize != VSP_UCD_SIZE) {
        reportError(ret, "Responsed driver data size mismatch.");
        IOConnectUnmapMemory(m_connection, input->command, mach_task_self(), (mach_vm_address_t) m_vspResponse);
        m_vspResponse = nullptr;
        return false;
    }

#ifdef VSP_DEBUG
    printStruct("asyncCall-Return", m_vspResponse);
#endif
    
    try {
        // Call Objective-C bridge function
        ConvertDataFromCPP((const void*)m_vspResponse, VSP_UCD_SIZE);
    }
    catch(...) {
        reportError(ret, "Exception in Swift callback detected!");
    }
    
    return true;
}

void VSPController::setConnection(io_connect_t connection)
{
    if (connection != m_connection) {
        m_connection = connection;
        if (connection == IO_OBJECT_NULL) {
            VSPDriverDisconnected();
        }
        else {
            VSPDriverConnected();
        }
    }
}
