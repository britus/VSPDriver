/*
See the LICENSE.txt file for this sample’s licensing information.

Abstract:
Implementations of C functions to perform calls to the driver and implement driver lifecycle callbacks.
*/
#include "VSPController.h"

#include <stdio.h>

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>

// If you don't know what value to use here, it should be identical to the
// IOUserClass value in your IOKitPersonalities. You can double check
// by searching with the `ioreg` command in your terminal. It will be of
// type "IOUserService" not "IOUserServer". The client Info.plist must
// contain:
// <key>com.apple.developer.driverkit.userclient-access</key>
// <array>
//     <string>VSPDriver</string>
// </array>
//
static const char* dextIdentifier = "VSPDriver";

static IONotificationPortRef globalNotificationPort = NULL;
static mach_port_t globalMachNotificationPort;
static CFRunLoopRef globalRunLoop = NULL;
static CFRunLoopSourceRef globalRunLoopSource = NULL;
static io_iterator_t globalDeviceAddedIter = IO_OBJECT_NULL;
static io_iterator_t globalDeviceRemovedIter = IO_OBJECT_NULL;

// MARK: Helpers

static inline void PrintArray(const uint64_t* ptr, const uint32_t length)
{
    printf("{ ");
    for (uint32_t idx = 0; idx < length; ++idx)
    {
        printf("%llu ", ptr[idx]);
    }
    printf("} \n");
}

static inline void PrintStruct(const TVSPControllerData* ptr)
{
    printf("{\n");
    printf("\t.context = %u,\n", ptr->context);
    printf("\t.status.code = %u,\n", ptr->status.code);
    printf("\t.status.message = %s,\n", ptr->status.message);
    printf("\t.command = %u,\n", ptr->command);
    printf("\t.parameter.flags = 0x%llx,\n", ptr->parameter.flags);
    printf("\t.ppl.sourceId = %u,\n", ptr->parameter.portLink.sourceId);
    printf("\t.ppl.targetId = %u,\n", ptr->parameter.portLink.targetId);
    printf("}\n");
}

static inline void PrintErrorDetails(kern_return_t ret)
{
    printf("\tSystem: 0x%02x\n", err_get_system(ret));
    printf("\tSubsystem: 0x%03x\n", err_get_sub(ret));
    printf("\tCode: 0x%04x\n", err_get_code(ret));
}

// MARK: C Constructors/Destructors

bool UserClientSetup(void* refcon)
{
    kern_return_t ret = kIOReturnSuccess;

    globalRunLoop = CFRunLoopGetCurrent();
    if (globalRunLoop == NULL)
    {
        fprintf(stderr, "Failed to initialize globalRunLoop.\n");
        return false;
    }
    CFRetain(globalRunLoop);

    globalNotificationPort = IONotificationPortCreate(kIOMainPortDefault);
    if (globalNotificationPort == NULL)
    {
        fprintf(stderr, "Failed to initialize globalNotificationPort.\n");
        UserClientTeardown();
        return false;
    }

    globalMachNotificationPort = IONotificationPortGetMachPort(globalNotificationPort);
    if (globalMachNotificationPort == 0)
    {
        fprintf(stderr, "Failed to initialize globalMachNotificationPort.\n");
        UserClientTeardown();
        return false;
    }

    globalRunLoopSource = IONotificationPortGetRunLoopSource(globalNotificationPort);
    if (globalRunLoopSource == NULL)
    {
        fprintf(stderr, "Failed to initialize globalRunLoopSource.\n");
        return false;
    }

    // Establish our notifications in the run loop, so we can get callbacks.
    CFRunLoopAddSource(globalRunLoop, globalRunLoopSource, kCFRunLoopDefaultMode);

    /// - Tag: SetUpMatchingNotification
    CFMutableDictionaryRef matchingDictionary = IOServiceNameMatching(dextIdentifier);
    if (matchingDictionary == NULL)
    {
        fprintf(stderr, "Failed to initialize matchingDictionary.\n");
        UserClientTeardown();
        return false;
    }
    matchingDictionary = (CFMutableDictionaryRef)CFRetain(matchingDictionary);
    matchingDictionary = (CFMutableDictionaryRef)CFRetain(matchingDictionary);

    ret = IOServiceAddMatchingNotification(globalNotificationPort,
                                           kIOFirstMatchNotification,
                                           matchingDictionary,
                                           DeviceAdded, refcon,
                                           &globalDeviceAddedIter);
    if (ret != kIOReturnSuccess)
    {
        fprintf(stderr, "Add matching notification failed with error: 0x%08x.\n", ret);
        UserClientTeardown();
        return false;
    }
    DeviceAdded(refcon, globalDeviceAddedIter);

    ret = IOServiceAddMatchingNotification(globalNotificationPort,
                                           kIOTerminatedNotification,
                                           matchingDictionary,
                                           DeviceRemoved, refcon,
                                           &globalDeviceRemovedIter);
    if (ret != kIOReturnSuccess)
    {
        fprintf(stderr, "Add termination notification failed with error: 0x%08x.\n", ret);
        UserClientTeardown();
        return false;
    }
    DeviceRemoved(refcon, globalDeviceRemovedIter);

    return true;
}

void UserClientTeardown(void)
{
    if (globalRunLoopSource)
    {
        CFRunLoopRemoveSource(globalRunLoop, globalRunLoopSource, kCFRunLoopDefaultMode);
        globalRunLoopSource = NULL;
    }

    if (globalNotificationPort)
    {
        IONotificationPortDestroy(globalNotificationPort);
        globalNotificationPort = NULL;
        globalMachNotificationPort = 0;
    }

    if (globalRunLoop)
    {
        CFRelease(globalRunLoop);
        globalRunLoop = NULL;
    }

    globalDeviceAddedIter = IO_OBJECT_NULL;
    globalDeviceRemovedIter = IO_OBJECT_NULL;
}

// MARK: Asynchronous Events

// ----------------------------------------------------------------
//
//
void DeviceAdded(void* refcon, io_iterator_t iterator)
{
    kern_return_t ret = kIOReturnNotFound;
    io_connect_t connection = IO_OBJECT_NULL;
    io_service_t device = IO_OBJECT_NULL;
    //bool attemptedToMatchDevice = false;
    bool clientFound = false;
    
    while ((device = IOIteratorNext(iterator)) != IO_OBJECT_NULL)
    {
        //attemptedToMatchDevice = true;
        io_name_t path;
        io_name_t deviceName;
        if (IORegistryEntryGetName(device, deviceName) == kIOReturnSuccess) {
            printf("DeviceAdded() name: %s\n", deviceName);
        }
        if (IORegistryEntryGetPath(device, kIOServicePlane, path)== kIOReturnSuccess) {
            printf("DeviceAdded() plane: %s path: %s\n", kIOServicePlane, path);
        }
        if (IORegistryEntryGetNameInPlane(device, kIOServicePlane, deviceName) == kIOReturnSuccess) {
            printf("DeviceAdded() name in plane: %s\n", deviceName);
        }

        // Open a connection to this user client as a server
        // to that client, and store the instance in "service"
        if ((ret = IOServiceOpen(device, mach_task_self_, 0, &connection)) != kIOReturnSuccess) {
            if (ret == kIOReturnNotPermitted) {
                fprintf(stdout, "DeviceAdded() Operation 'IOServiceOpen' not permitted.\n");
            }
            IOObjectRelease(device);
            continue;
        }

        fprintf(stdout, "DeviceAdded() Opened connection to dext.\n");

        SwiftDeviceAdded(refcon, connection);
        IOObjectRelease(device);
        clientFound = true;
        ret = kIOReturnSuccess;
    }

    if (!clientFound) {
        fprintf(stderr, "DeviceAdded() Failed opening connection to dext with error: 0x%08x.\n", ret);
    }
}

// ----------------------------------------------------------------
//
//
void DeviceRemoved(void* refcon, io_iterator_t iterator)
{
    io_service_t device = IO_OBJECT_NULL;

    while ((device = IOIteratorNext(iterator)) != IO_OBJECT_NULL)
    {
        fprintf(stdout, "Closed connection to dext.\n");
        IOObjectRelease(device);
        SwiftDeviceRemoved(refcon);
    }
}

// ----------------------------------------------------------------
// For more detail on this callback format, view the format of:
// IOAsyncCallback, IOAsyncCallback0, IOAsyncCallback1, IOAsyncCallback2
// Note that the variant of IOAsyncCallback called is based on the
// number of arguments being returned
// 0 - IOAsyncCallback0
// 1 - IOAsyncCallback1
// 2 - IOAsyncCallback2
// 3+ - IOAsyncCallback
// This is an example of the "IOAsyncCallback" format.
// refcon will be the value you placed in asyncRef[kIOAsyncCalloutRefconIndex]
void AsyncCallback(void* refcon, IOReturn result, void** args, UInt32 numArgs)
{
    uint64_t* arrArgs = (uint64_t*)args;
    
    for (uint8_t index = 0; index < 3; index++) {
        printf("AsyncCallback #%d: ------------------------- \n", index);
        TVSPControllerData* response = (TVSPControllerData*)(arrArgs + index);
        PrintStruct(response);
    }
 
    SwiftAsyncCallback(refcon, result, args, numArgs);
}

// ----------------------------------------------------------------
//
//
static inline bool doAsyncCall(void* refcon, io_connect_t connection, const TVSPControllerData* input)
{
    io_async_ref64_t asyncRef = {};

    // Establish our "AsyncCallback" function as the function that will be called
    // by our Dext when it calls its "AsyncCompletion" function.
    // We'll use kIOAsyncCalloutFuncIndex and kIOAsyncCalloutRefconIndex
    // to define the parameters for our async callback. This is your callback
    // function. Check the definition for more details.
    asyncRef[kIOAsyncCalloutFuncIndex]   = (io_user_reference_t) AsyncCallback;
    
    // Use this for context on the return. We'll pass the refcon so we can
    // talk back to the view model.
    asyncRef[kIOAsyncCalloutRefconIndex] = (io_user_reference_t) refcon;

    kern_return_t ret = kIOReturnSuccess;

    size_t resultSize = VSP_UCD_SIZE;
    TVSPControllerData result = { };

    ret = IOConnectCallAsyncStructMethod(connection,
                                         input->command,
                                         globalMachNotificationPort,
                                         asyncRef,
                                         kIOAsyncCalloutCount,
                                         input, VSP_UCD_SIZE, &result, &resultSize);
    if (ret != kIOReturnSuccess) {
        printf("IOConnectCallStructMethod failed with error: 0x%08x.\n", ret);
        PrintErrorDetails(ret);
    }

    printf("doAsyncCall-Request: -------------------------- \n");
    PrintStruct(input);
    
    printf("doAsyncCall-Return.: ------------------------- \n");
    PrintStruct(&result);

    printf("Async actions can now be executed.\n");
    printf("Please wait for the callback...\n");

    return (ret == kIOReturnSuccess);
}

// ----------------------------------------------------------------
//
//
bool GetPortList(void* refcon, io_connect_t connection)
{
    TVSPControllerData input = {};
    input.context = vspContextPort;
    input.command = vspControlGetPortList;

    return doAsyncCall(refcon, connection, &input);
}

// ----------------------------------------------------------------
//
//
bool LinkPorts(void* refcon, io_connect_t connection, const uint8_t source, const uint8_t target)
{
    TVSPControllerData input = {};
    input.context = vspContextPort;
    input.command = vspControlLinkPorts;
    input.parameter.portLink.sourceId = source;
    input.parameter.portLink.targetId = target;

    return doAsyncCall(refcon, connection, &input);
}

// ----------------------------------------------------------------
//
//
bool UnlinkPorts(void* refcon, io_connect_t connection, const uint8_t source, const uint8_t target)
{
    TVSPControllerData input = {};
    input.context = vspContextPort;
    input.command = vspControlUnlinkPorts;
    input.parameter.portLink.sourceId = source;
    input.parameter.portLink.targetId = target;

    return doAsyncCall(refcon, connection, &input);
}

// ----------------------------------------------------------------
//
//
bool EnableChecks(void* refcon, io_connect_t connection, const uint8_t port)
{
    TVSPControllerData input = {};
    input.context = vspContextPort;
    input.command = vspControlEnableChecks;
    input.parameter.portLink.sourceId = port;
    input.parameter.portLink.targetId = port;
    input.parameter.flags = 0x01;

    return doAsyncCall(refcon, connection, &input);
}

// ----------------------------------------------------------------
//
//
bool EnableTrace(void* refcon, io_connect_t connection, const uint8_t port)
{
    TVSPControllerData input = {};
    input.context = vspContextPort;
    input.command = vspControlEnableTrace;
    input.parameter.portLink.sourceId = port;
    input.parameter.portLink.targetId = port;
    input.parameter.flags = 0x01;

    return doAsyncCall(refcon, connection, &input);
}
