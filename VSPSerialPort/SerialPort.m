// ********************************************************************
// VSPClient - serial port access
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
#import <Foundation/Foundation.h>
#import "SerialPort.h"
#include <sys/types.h>
#include <sys/stat.h>
//#include <fcntl.h>
//#include <unistd.h>
//#include <termios.h>
//#include <errno.h>
#include <string.h>
#include <time.h>
#import <sys/ioctl.h>
#import <termios.h>
#import <fcntl.h>
#import <unistd.h>
#import <errno.h>

@interface SerialPort ()
@property (nonatomic, assign) int fdPort;
@property (nonatomic, strong) dispatch_queue_t readerQueue;
@property (nonatomic, strong) dispatch_queue_t writeQueue;
@property (nonatomic, strong) NSMutableData *receivedData;
@property (nonatomic, assign) SerialPortState currentState;
@property (nonatomic, strong) NSString *lastErrorMessage;
@property (nonatomic, assign) BOOL isWriting;
@end

@implementation SerialPort

- (instancetype)initWithPortPath:(NSString *)portPath {
    if ((self = [super init])) {
        _parameters = [[SerialPortParameters alloc]
                       initWith:115200
                       dataBits:8
                       stopBits:1
                       parity:0
                       flowCtrl:0];
        _currentState = SerialPortStateDisconnected;
        _readerQueue = NULL; //dispatch_queue_create("vspReaderQueue", DISPATCH_QUEUE_SERIAL);
        _writeQueue = NULL; //dispatch_queue_create("vspWriterQueue", DISPATCH_QUEUE_SERIAL);
        _receivedData = [[NSMutableData alloc] init];
        _isWriting = NO;
        _portPath = portPath;
        _fdPort = -1;
    }
    return self;
}

- (instancetype)initWithPathAndParameters:(NSString *)portPath
                        parameters:(SerialPortParameters*)parameters {
    if ((self = [self initWithPortPath:portPath])) {
        _parameters = parameters;
    }
    return self;
}

+ (NSArray<NSString *> *)getAvailableSerialPorts {
    NSMutableArray<NSString *> *ports = [[NSMutableArray alloc] init];
    kern_return_t result;

    // Get the master port for IOKit
    mach_port_t masterPort = kIOMainPortDefault;

    // Create a matching dictionary for all serial BSD devices
    CFMutableDictionaryRef matchingDict = IOServiceMatching(kIOSerialBSDServiceValue);
    if (!matchingDict) {
        NSLog(@"Failed to create matching dictionary for serial devices");
        return ports;
    }

    // We are interested in callout devices (e.g., /dev/cu.*)
    CFDictionarySetValue(matchingDict, CFSTR(kIOSerialBSDTypeKey), CFSTR(kIOSerialBSDAllTypes));

    io_iterator_t serviceIterator;
    result = IOServiceGetMatchingServices(masterPort, matchingDict, &serviceIterator);
    if (result != KERN_SUCCESS) {
        NSLog(@"Failed to get matching services: %d", result);
        return ports;
    }

    io_object_t service;
    while ((service = IOIteratorNext(serviceIterator))) {
        NSString *devicePath;
        devicePath = [self getDevicePathForService:service withKey:CFSTR(kIOCalloutDeviceKey)];
        if (devicePath.length > 0) {
            [ports addObject:devicePath];
        }
        devicePath = [self getDevicePathForService:service withKey:CFSTR(kIODialinDeviceKey)];
        if (devicePath.length > 0) {
            [ports addObject:devicePath];
        }
        IOObjectRelease(service);
    }

    IOObjectRelease(serviceIterator);
    return ports;
}

// Helper: retrieve kIOCalloutDeviceKey property from IORegistry
+ (NSString *)getDevicePathForService:(io_service_t)service withKey:(CFStringRef)key {
    CFTypeRef cfPath = IORegistryEntryCreateCFProperty(service, key, kCFAllocatorDefault, 0);
    if (cfPath) {
        NSString *path = (__bridge_transfer NSString *)cfPath;
        return path;
    }
    return @"";
}

- (NSError *)createErrorWithCode:(NSInteger)code
                         message:(NSString *)message
                       errorType:(SerialPortErrorType)errorType {
    NSDictionary *userInfo = @{
        NSLocalizedDescriptionKey: message,
        @"ErrorType": @(errorType),
        @"ErrorCode": @(code)
    };
    return [NSError errorWithDomain:@"SerialPortErrorDomain"
                               code:code
                           userInfo:userInfo];
}

- (void)fireErrorEvent:(NSError *)error withType:(SerialPortErrorType)errorType {
    [self notifyDelegateError:error withType:errorType];
}

- (void)fireConnectError:(NSString *)errorMessage errorType:(SerialPortErrorType)errorType {
    // Validate parameters
    if (!errorMessage) {
        return;
    }
    
    // Log the error
    NSLog(@"Serial Port Error - %@ (Error Type: %ld)", errorMessage, (long)errorType);
    
    NSError *error = [self createErrorWithCode:errno
              message:[NSString stringWithUTF8String:strerror(errno)]
            errorType:(errno == EACCES) ? SerialPortErrorTypePermissionDenied :
                      (errno == ENOENT) ? SerialPortErrorTypeNotFound : errorType];
    [self fireErrorEvent:error withType:errorType];
}

- (void)handleConnectionError:(NSString *)message withType:(SerialPortErrorType)errorType {
    NSError *error = [self createErrorWithCode:ENODEV message:message errorType:errorType];
    [self fireErrorEvent:error withType:errorType];
}

- (void)updateState:(SerialPortState)state {
    if (self.currentState != state) {
        self.currentState = state;
        [self notifyDelegateStateChanged:state];
    }
}

- (NSString *)stateDescription:(SerialPortState)state {
    switch (state) {
        case SerialPortStateDisconnected:
            return @"Disconnected";
        case SerialPortStateConnecting:
            return @"Connecting";
        case SerialPortStateConnected:
            return @"Connected";
        case SerialPortStateError:
            return @"Error";
        case SerialPortStateDisconnecting:
            return @"Disconnecting";
        default:
            return @"Unknown";
    }
}

- (void)configureSerialPort {
    
    //cc_t vmin = 1;
    //cc_t vtime = 0;

    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    
    if (tcgetattr(self.fdPort, &tty) != 0) {
        [self fireConnectError:@"Failed to get terminal attributes"
                             errorType:SerialPortErrorTypeConfigurationFailed];
        [self updateState:SerialPortStateError];
        return;
    }
    
    // Configure serial port based on settings
    speed_t speed = B9600;
    switch (self.parameters.baudRate) {
        case 50:
            speed = B50;
            break;
        case 75:
            speed = B75;
            break;
        case 110:
            speed = B110;
            break;
        case 134:
            speed = B134;
            break;
        case 150:
            speed = B150;
            break;
        case 200:
            speed = B200;
            break;
        case 300:
            speed = B300;
            break;
        case 600:
            speed = B600;
            break;
        case 1200:
            speed = B1200;
            break;
        case 1800:
            speed = B1800;
            break;
        case 2400:
            speed = B2400;
            break;
        case 4800:
            speed = B4800;
            break;
        case 9600:
            speed = B9600;
            break;
        case 19200:
            speed = B19200;
            break;
        case 38400:
            speed = B38400;
            break;
        case 57600:
            speed = B57600;
            break;
        case 115200:
            speed = B115200;
            break;
        case 230400:
            speed = B230400;
            break;
    }
    
    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);
    
    // Set up parity, data bits, stop bits
    tty.c_cflag &= ~PARENB; // No parity
    tty.c_cflag &= ~CSTOPB; // 1 stop bit
    tty.c_cflag &= ~CSIZE;  // Clear data size bits
    
    switch (self.parameters.parity) {
        case 0: // None
            break;
        case 1: // Odd
            tty.c_cflag |= PARENB;
            tty.c_cflag |= PARODD;
            break;
        case 2: // Even
            tty.c_cflag |= PARENB;
            tty.c_cflag &= ~PARODD;
            break;
    }
    
    switch (self.parameters.dataBits) {
        case 5:
            tty.c_cflag |= CS5;
            break;
        case 6:
            tty.c_cflag |= CS6;
            break;
        case 7:
            tty.c_cflag |= CS7;
            break;
        case 8:
            tty.c_cflag |= CS8;
            break;
        default:
            tty.c_cflag |= CS8;
            break;
    }
    
    switch (self.parameters.stopBits) {
        case 1:
            tty.c_cflag &= ~CSTOPB;
            break;
        case 2:
            tty.c_cflag |= CSTOPB;
            break;
    }

    // Raw mode
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL);
    tty.c_oflag &= ~OPOST;
    tty.c_cflag &= ~CRTSCTS;

    // Configure hardware flow control
    if (self.parameters.flowCtrl == 1) {
        tty.c_cflag |= CRTSCTS; // Enable hardware flow control (RTS/CTS)
    }
    // Configure XON/XOFF
    else if (self.parameters.flowCtrl == 2) {
        tty.c_cflag |= IXON | IXOFF; // Enable XON/XOFF
    }
    
    if (tcsetattr(self.fdPort, TCSANOW, &tty) != 0) {
        [self fireConnectError:@"Failed to configure terminal"
                             errorType:SerialPortErrorTypeConfigurationFailed];
        [self updateState:SerialPortStateError];
        return;
    }
}

- (BOOL)connect {
    if (self.currentState == SerialPortStateConnected) {
        return YES;
    }
    
    if (self.fdPort != -1) {
        [self fireConnectError:@"Port already open"
                             errorType:SerialPortErrorTypeAlreadyOpen];
        return NO;
    }
    

    const char *portPath = [self.portPath UTF8String];
    char readerQueueName[1024];
    char writerQueueName[1024];

    // Create queue names with port path included
    snprintf(readerQueueName, sizeof(readerQueueName)-1, "vspReaderQueue_%s", portPath);
    snprintf(writerQueueName, sizeof(writerQueueName)-1, "vspWriterQueue_%s", portPath);
    _readerQueue = dispatch_queue_create(readerQueueName, DISPATCH_QUEUE_SERIAL);
    _writeQueue = dispatch_queue_create(writerQueueName, DISPATCH_QUEUE_SERIAL);

    int fdflags = (O_RDWR /*| O_NOCTTY*/ | O_NONBLOCK);
    
    if ((self.fdPort = open(portPath, fdflags)) == -1) {
        NSError *error = [self createErrorWithCode:errno
                                             message:[NSString stringWithUTF8String:strerror(errno)]
                                            errorType:(errno == EACCES) ? SerialPortErrorTypePermissionDenied :
                                                     (errno == ENOENT) ? SerialPortErrorTypeNotFound :
                                                     SerialPortErrorTypeUnknown];
        [self fireConnectError:[error localizedDescription]
                             errorType:error.code == EACCES ? SerialPortErrorTypePermissionDenied :
                                      errno == ENOENT ? SerialPortErrorTypeNotFound :
                                      SerialPortErrorTypeUnknown];
        [self updateState:SerialPortStateError];
        return NO;
    }
    
    [self configureSerialPort];
    [self updateState:SerialPortStateConnected];
    [self startReading];
    
    return YES;
}

- (void)disconnect {
    [self updateState:SerialPortStateDisconnecting];
    if (self.fdPort != -1) {
        close(self.fdPort);
        self.fdPort = -1;
    }
    [self updateState:SerialPortStateDisconnected];
}

- (BOOL)sendData:(NSData *)data {
    __block BOOL success = NO;
    
    if (!self.writeQueue) {
        NSError *error = [self createErrorWithCode:EINVAL
                  message:[NSString stringWithUTF8String:strerror(errno)]
                errorType:SerialPortErrorTypeNoQueue];
        [self fireErrorEvent:error withType:SerialPortErrorTypeNoQueue];
        return NO;
    }

    dispatch_sync(self.writeQueue, ^{
        if (self.fdPort == -1 || self.currentState != SerialPortStateConnected) {
            NSError *error = [self createErrorWithCode:ENOTCONN
                                               message:@"Port not connected"
                                             errorType:SerialPortErrorTypeWriteFailed];
            [self fireErrorEvent:error withType:SerialPortErrorTypeWriteFailed];
            return;
        }
        
        self.isWriting = YES;
        
        const uint8_t *bytes = [data bytes];
        size_t length = [data length];
        
        ssize_t written = write(self.fdPort, bytes, length);
        
        if (written < 0) {
            NSError *error = [self
                        createErrorWithCode:errno
                                    message:[NSString stringWithUTF8String:strerror(errno)]
                                  errorType:SerialPortErrorTypeWriteFailed];
            [self fireErrorEvent:error withType:SerialPortErrorTypeWriteFailed];
            self.isWriting = NO;
            return;
        }
        
        if (written != (ssize_t)length) {
            NSError *error = [self
                        createErrorWithCode:EBADF
                                    message:@"Partial write occurred"
                                  errorType:SerialPortErrorTypeWriteFailed];
            [self fireErrorEvent:error withType:SerialPortErrorTypeWriteFailed];
            self.isWriting = NO;
            return;
        }
        
        self.isWriting = NO;
        success = YES;
    });
      
    return success;
}

- (void)startReading {
    if (!self.readerQueue) {
        NSError *error = [self createErrorWithCode:EINVAL
                  message:[NSString stringWithUTF8String:strerror(errno)]
                errorType:SerialPortErrorTypeNoQueue];
        [self fireErrorEvent:error withType:SerialPortErrorTypeReadTimeout];
        return;
    }
    dispatch_async(self.readerQueue, ^{
        uint8_t buffer[1024];
        ssize_t bytesRead;
        
        while (self.fdPort != -1 && self.currentState == SerialPortStateConnected) {
            if (!self.isWriting) {
                bytesRead = read(self.fdPort, buffer, sizeof(buffer));
            } else {
                bytesRead = 0;
            }
            
            if (bytesRead < 0) {
                // Handle read error
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // Timeout - continue reading
                    continue;
                } else {
                    // Actual error occurred
                    NSError *error = [self createErrorWithCode:errno
                              message:[NSString stringWithUTF8String:strerror(errno)]
                            errorType:SerialPortErrorTypeReadTimeout];
                    [self fireErrorEvent:error withType:SerialPortErrorTypeReadTimeout];
                    break;
                }
            } else if (bytesRead == 0) {
                // No data available, continue
                continue;
            } else if (bytesRead > 0) {
                // Data received
                NSData *data = [NSData dataWithBytes:buffer length:(size_t)bytesRead];
                [self notifyDelegateReceivedData:data];
            }
        }
        
        // If we exit the loop, disconnect
        if (self.currentState != SerialPortStateDisconnected) {
            [self disconnect];
        }
    });
}

- (void)setParameters:(SerialPortParameters *)newParams {
    if (self.parameters.baudRate != newParams.baudRate) {
        self.parameters.baudRate = newParams.baudRate;
        if ([self.delegate respondsToSelector:@selector(serialPort:didChangeBaudRate:)]) {
            [self.delegate serialPort:self didChangeBaudRate:newParams.baudRate];
        }
    }
    if (self.parameters.dataBits != newParams.dataBits) {
        self.parameters.dataBits = newParams.dataBits;
        if ([self.delegate respondsToSelector:@selector(serialPort:didChangeDataBits:)]) {
            [self.delegate serialPort:self didChangeDataBits:newParams.dataBits];
        }
    }
    if (self.parameters.stopBits != newParams.stopBits) {
        self.parameters.stopBits = newParams.stopBits;
        if ([self.delegate respondsToSelector:@selector(serialPort:didChangeStopBits:)]) {
            [self.delegate serialPort:self didChangeStopBits:newParams.stopBits];
        }
    }
    if (self.parameters.parity != newParams.parity) {
        self.parameters.parity = newParams.parity;
        if ([self.delegate respondsToSelector:@selector(serialPort:didChangeParity:)]) {
            [self.delegate serialPort:self didChangeParity:newParams.parity];
        }
    }
    if (self.parameters.flowCtrl != newParams.flowCtrl) {
        self.parameters.flowCtrl = newParams.flowCtrl;
        if ([self.delegate respondsToSelector:@selector(serialPort:didChangeFlowControl:)]) {
            [self.delegate serialPort:self didChangeFlowControl:newParams.flowCtrl];
        }
    }
}

- (void)sendFileAtPath:(NSString *)filePath chunkSize:(NSUInteger)chunkSize {
    NSData *fileData = [NSData dataWithContentsOfFile:filePath];
    if (!fileData) {
        NSLog(@"Failed to read file at path: %@", filePath);
        return;
    }

    const uint8_t *bytes = (const uint8_t *)[fileData bytes];
    NSUInteger totalLength = [fileData length];

    for (NSUInteger i = 0; i < totalLength; i += chunkSize) {
        NSUInteger length = MIN(chunkSize, totalLength - i);
        NSData *chunk = [NSData dataWithBytes:bytes + i length:length];
        // Assuming you have a sendData method to send the chunk
        [self sendData:chunk]; // This should be implemented in your class
    }
}

- (void)monitorPinoutSignals {
    int status;
    while (self.currentState == SerialPortStateConnected) {
        if (ioctl(self.fdPort, TIOCMGET, &status) == 0) {
            BOOL DCD = (status & TIOCM_CAR);
            BOOL DTR = (status & TIOCM_DTR);
            BOOL DSR = (status & TIOCM_DSR);
            BOOL RTS = (status & TIOCM_RTS);
            BOOL CTS = (status & TIOCM_CTS);
            BOOL RI = (status & TIOCM_RI);

            // Notify delegate
            if ([self.delegate respondsToSelector:@selector(serialPort:didUpdatePinoutSignals:DTR:DSR:RTS:CTS:RI:)]) {
                [self.delegate serialPort:self didUpdatePinoutSignals:DCD DTR:DTR DSR:DSR RTS:RTS CTS:CTS RI:RI];
            }
        }
        [NSThread sleepForTimeInterval:0.5]; // Poll every 500ms
    }
}

// Notification methods
- (void)notifyDelegateStateChanged:(SerialPortState)state {
    if ([self.delegate respondsToSelector:@selector(serialPortStateChanged:)]) {
        [self.delegate serialPortStateChanged:state];
    }
}

- (void)notifyDelegateReceivedData:(NSData *)data {
    if ([self.delegate respondsToSelector:@selector(serialPortDidReceiveData:)]) {
        [self.delegate serialPortDidReceiveData:data];
    }
}

- (void)notifyDelegateConnect {
    if ([self.delegate respondsToSelector:@selector(serialPortDidConnect)]) {
        [self.delegate serialPortDidConnect];
    }
}

- (void)notifyDelegateDisconnect:(NSError *)error {
    if ([self.delegate respondsToSelector:@selector(serialPortDidDisconnect:)]) {
        [self.delegate serialPortDidDisconnect:error];
    }
}

- (void)notifyDelegateError:(NSError *)error withType:(SerialPortErrorType)errorType {
    if ([self.delegate respondsToSelector:@selector(serialPortDidError:withType:)]) {
        [self.delegate serialPortDidError:error withType:errorType];
    }
}

@end
