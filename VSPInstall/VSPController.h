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

extern void SwiftAsyncCallback(void* refcon, IOReturn result, void** args, UInt32 numArgs);
extern void SwiftDeviceAdded(void* refcon, io_connect_t connection);
extern void SwiftDeviceRemoved(void* refcon);

void AsyncCallback(void* refcon, IOReturn result, void** args, UInt32 numArgs);

void DeviceAdded(void* refcon, io_iterator_t iterator);
void DeviceRemoved(void* refcon, io_iterator_t iterator);

bool UserClientSetup(void* refcon);
void UserClientTeardown(void);

bool UncheckedScalar(io_connect_t connection);
bool UncheckedStruct(io_connect_t connection);
bool UncheckedLargeStruct(io_connect_t connection);
bool CheckedScalar(io_connect_t connection);
bool CheckedStruct(io_connect_t connection);
bool AssignAsyncCallback(void* refcon, io_connect_t connection);
bool SubmitAsyncRequest(io_connect_t connection);

#endif /* VSPController_h */
