//
//  VSPDriverClient.hpp
//  VSPDriver
//
//  Created by Björn Eschrich on 13.02.25.
//

#ifndef VSPDriverClient_h
#define VSPDriverClient_h

bool UserClientSetup(void* refcon);
void UserClientTeardown(void);
bool GetPortList(void* refcon, io_connect_t connection);
bool LinkPorts(void* refcon, io_connect_t connection, const uint8_t source, const uint8_t target);
bool UnlinkPorts(void* refcon, io_connect_t connection, const uint8_t source, const uint8_t target);
bool EnableChecks(void* refcon, io_connect_t connection, const uint8_t port);
bool EnableTrace(void* refcon, io_connect_t connection, const uint8_t port);

#endif /* VSPDriverClient_h */
