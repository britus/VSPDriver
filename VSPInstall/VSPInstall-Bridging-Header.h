//
//  Use this file to import your target's public headers that you would like to expose to Swift.
//
#import <stdio.h>
#import <IOKit/IOTypes.h>
#import <IOKit/IOKitLib.h>
#import <IOKit/serial/IOSerialKeys.h>
#import <IOKit/serial/ioss.h>

bool UserClientSetup(void* refcon);
void UserClientTeardown(void);
bool GetPortList(void* refcon, io_connect_t connection);
bool LinkPorts(void* refcon, io_connect_t connection, const uint8_t source, const uint8_t target);
bool UnlinkPorts(void* refcon, io_connect_t connection, const uint8_t source, const uint8_t target);
bool EnableChecks(void* refcon, io_connect_t connection, const uint8_t port);
bool EnableTrace(void* refcon, io_connect_t connection, const uint8_t port);
