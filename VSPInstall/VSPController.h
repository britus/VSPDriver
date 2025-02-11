//
//  VSPController.h
//  VSPInstall
//
//  Created by Björn Eschrich on 10.02.25.
//

#ifndef VSPController_h
#define VSPController_h

#include <stdio.h>
#include <IOKit/IOTypes.h>

typedef enum {
    vspContextPing = 1,
    vspContextPort = 2,
    vspContextResult = 3,
    vspContextError = 4,
} TVSPUserContext;

typedef enum {
    vspControlGetStatus,
    vspControlGetPortList,
    vspControlLinkPorts,
    vspControlUnlinkPorts,
    vspControlEnableChecks,
    vspControlEnableTrace,
    // Has to be last
    vspLastCommand,
} TVSPControlCommand;

typedef struct {
    uint8_t sourceId;
    uint8_t targetId;
} TPortLink;

#ifndef VSP_UCD_MESSAGE_SIZE
#define VSP_UCD_MESSAGE_SIZE 127
#endif

typedef struct {
    /* In whitch context calld */
    TVSPUserContext context;
    /* User client command */
    TVSPControlCommand command;
    /* Command parameters */
    struct Parameter {
        /* command flags */
        uint64_t flags;
        /* port parameters */
        TPortLink portLink;
    } parameter;
    /* Available serial ports */
    struct PortList {
        uint8_t count;
        uint8_t list[16];
    } ports;
    /* Command status response */
    struct Status {
        uint32_t code;
        uint64_t flags;
    } status;
} TVSPControllerData;

#ifndef VSP_UCD_SIZE
#define VSP_UCD_SIZE sizeof(TVSPControllerData)
#endif

extern void SwiftAsyncCallback(void* refcon, IOReturn result, void** args, UInt32 numArgs);
extern void SwiftDeviceAdded(void* refcon, io_connect_t connection);
extern void SwiftDeviceRemoved(void* refcon);

void AsyncCallback(void* refcon, IOReturn result, void** args, UInt32 numArgs);

void DeviceAdded(void* refcon, io_iterator_t iterator);
void DeviceRemoved(void* refcon, io_iterator_t iterator);

bool UserClientSetup(void* refcon);
void UserClientTeardown(void);

bool GetPortList(void* refcon, io_connect_t connection);
bool LinkPorts(void* refcon, io_connect_t connection, const uint8_t source, const uint8_t target);
bool UnlinkPorts(void* refcon, io_connect_t connection, const uint8_t source, const uint8_t target);
bool EnableChecks(void* refcon, io_connect_t connection, const uint8_t port);
bool EnableTrace(void* refcon, io_connect_t connection, const uint8_t port);

#endif /* VSPController_h */
