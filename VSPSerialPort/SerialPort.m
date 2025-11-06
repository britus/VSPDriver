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
@property (nonatomic, assign) int fdDevice;
@property (nonatomic, strong) dispatch_queue_t readerQueue;
@property (nonatomic, strong) dispatch_queue_t writeQueue;
@property (nonatomic, strong) dispatch_queue_t pinsigQueue;
@property (nonatomic, strong) NSLock *portAccessLock;
@property (nonatomic, strong) NSMutableData *receivedData;
@property (nonatomic, assign) SerialPortState currentState;
@property (nonatomic, strong) NSString *lastErrorMessage;
@property (nonatomic, assign) io_connect_t serialPortRef;
@property (nonatomic, assign) IONotificationPortRef notificationPort;
@property (nonatomic, assign) CFRunLoopSourceRef runLoopSource;
@end

#ifdef DEBUG // For debug purposes only (O.o)
void removeFile(NSString* filename) {
    NSString* homeDirectory = NSHomeDirectory();
    NSString* filePath = [homeDirectory stringByAppendingPathComponent:filename];
    NSFileManager *fileManager = [NSFileManager defaultManager];
    if ([fileManager fileExistsAtPath:filePath]) {
        NSError *error;
        BOOL success = [fileManager removeItemAtPath:filePath error:&error];
        if (!success) {
            NSLog(@"Error deleting file: %@\n%@", filename, error.localizedDescription);
        }
    }
}
void writeReceivedData(NSData* data, size_t bufferSize, NSString* filename) {
    NSString* homeDirectory = NSHomeDirectory();
    NSString* filePath = [homeDirectory stringByAppendingPathComponent:filename];
    BOOL fileExists = [[NSFileManager defaultManager] fileExistsAtPath:filePath];
    if (fileExists) {
        //NSError* error;
        NSFileHandle* fileHandle = [NSFileHandle fileHandleForWritingAtPath:filePath];
        if (fileHandle) {
            [fileHandle seekToEndOfFile];
            [fileHandle writeData:data];
            [fileHandle closeFile];
        } else {
            NSLog(@"Failed to open file: %@", filePath);
        }
    } else {
        BOOL success = [data writeToFile:filePath atomically:YES];
        if (!success) {
            NSLog(@"Failed to create file: %@", filePath);
        }
    }
}
#endif

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
        _pinsigQueue = NULL;
        _receivedData = [[NSMutableData alloc] init];
        _portAccessLock = [[NSLock alloc] init];
        _portPath = portPath;
        _fdDevice = -1;
        _serialPortRef = 0;
        _notificationPort = MACH_PORT_NULL;
        _runLoopSource = NULL;
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
    return NULL;
}

- (NSString *)getBsdDevicePath:(io_service_t)service {
    NSString *path = @"";
    
    CFStringRef key = CFSTR(kIOBSDNameKey);
    CFTypeRef property = IORegistryEntryCreateCFProperty(
        service,
        key,
        kCFAllocatorDefault,
        0
    );
    
    if (property) {
        NSString *bsdName = (__bridge NSString *)property;
        path = [NSString stringWithFormat:@"/dev/%@", bsdName];
        CFRelease(property);
    }
    
    return path;
}

// Callback function for device removal
void deviceRemovalCallback(
    void *refCon,
    io_service_t service,
    natural_t messageType,
    void *messageArgument
) {
    SerialPort *monitor = (__bridge SerialPort *)refCon;
    
    // Handle device removal
    NSLog(@"Serial port device removed: %@", monitor.portPath);
    [monitor disconnect];
}

- (void)setupNotification:(io_service_t)service {
    IONotificationPortRef port = IONotificationPortCreate(kIOMainPortDefault);
    
    CFRunLoopSourceRef runLoopSource = IONotificationPortGetRunLoopSource(port);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopDefaultMode);
    
    // Create notification for device removal
    io_object_t notification = 0;
    kern_return_t result = IOServiceAddInterestNotification(
        port,
        service,
        kIOGeneralInterest,
        deviceRemovalCallback,
        (__bridge void *)self,
        &notification
    );
    
    if (result != KERN_SUCCESS) {
        NSLog(@"Failed to add device removal notification");
    }
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
    [self disconnect];
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

- (void)configureSerialPort:(int)fd {
    
    //cc_t vmin = 1;
    //cc_t vtime = 0;

    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    
    if (tcgetattr(fd, &tty) != 0) {
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
    
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        [self fireConnectError:@"Failed to configure terminal"
                             errorType:SerialPortErrorTypeConfigurationFailed];
        [self updateState:SerialPortStateError];
        return;
    }
}

// Method to create queues with custom names
- (dispatch_queue_t)createQueueWithName:(NSString *)baseName
                                filePath:(NSString *)filePath
                              attributes:(dispatch_queue_attr_t)attributes {
    
    // Generate unique queue name based on base name and device name
    NSString *devName = [filePath lastPathComponent];
    NSString *queueName = [NSString stringWithFormat:@"%@_%@", baseName, devName];
    const char* name = [queueName UTF8String];
    return dispatch_queue_create(name, attributes);
}

// Method to get the call-in device path (tty)
- (NSString *)getCallInPath:(NSString *)filePath {
    // Extract the basename and replace "cu." with "tty."
    NSString *basename = [filePath lastPathComponent];
    if ([basename hasPrefix:@"cu."]) {
        return [NSString stringWithFormat:@"%@/tty%@",
                [filePath stringByDeletingLastPathComponent],
                [basename substringFromIndex:3]];
    }
    return filePath;
}

// Method to get the call-out device path (cu)
- (NSString *)getCallOutPath:(NSString *)filePath {
    // Extract the basename and replace "tty." with "cu."
    NSString *basename = [filePath lastPathComponent];
    if ([basename hasPrefix:@"tty."]) {
        return [NSString stringWithFormat:@"%@/cu.%@",
                [filePath stringByDeletingLastPathComponent],
                [basename substringFromIndex:4]];
    }
    return filePath;
}

- (BOOL)startMonitoring {
    CFURLRef url = CFURLCreateWithFileSystemPath(
        kCFAllocatorDefault,
        (CFStringRef)self.portPath,
        kCFURLPOSIXPathStyle,
        false
    );
    
    if (!url) {
        NSLog(@"Invalid port path");
        return NO;
    }
    
    // Create notification port
    IONotificationPortRef port = IONotificationPortCreate(kIOMainPortDefault);
    self.notificationPort = port;
    
    // Create run loop source
    CFRunLoopSourceRef runLoopSource = IONotificationPortGetRunLoopSource(port);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopDefaultMode);
    
    // Set up matching dictionary for serial devices
    CFMutableDictionaryRef matchingDict = IOServiceMatching(kIOSerialBSDServiceValue);
    
    if (!matchingDict) {
        NSLog(@"Failed to create matching dictionary");
        return NO;
    }
    
    // Create iterator for matching services
    io_iterator_t iterator;
    kern_return_t result = IOServiceGetMatchingServices(kIOMainPortDefault, matchingDict, &iterator);
    
    if (result != KERN_SUCCESS) {
        NSLog(@"Failed to get matching services");
        return NO;
    }
    
    // Find our specific serial port device
    io_service_t service = 0;
    while ((service = IOIteratorNext(iterator)) != MACH_PORT_NULL) {
        // Check if this is our specific port
        NSString *devicePath = [self getBsdDevicePath:service];
        
        if ([devicePath isEqualToString:self.portPath]) {
            // Set up notification for this service
            [self setupNotification:service];
            break;
        }
        
        IOObjectRelease(service);
    }
    
    IOIteratorReset(iterator);
    IOObjectRelease(iterator);
    return YES;
}

- (BOOL)connect {
    if (self.currentState == SerialPortStateConnected) {
        return YES;
    }
    
    if (self.fdDevice != -1) {
        [self fireConnectError:@"Port already open"
                     errorType:SerialPortErrorTypeAlreadyOpen];
        return NO;
    }
    
    // Create queues with port name included
    self.pinsigQueue = [self createQueueWithName:@"vsp.psq"
                                    filePath:self.portPath
                                  attributes:DISPATCH_QUEUE_CONCURRENT];
    self.readerQueue = [self createQueueWithName:@"vsp.rdq"
                                    filePath:self.portPath
                                  attributes:DISPATCH_QUEUE_CONCURRENT];
    self.writeQueue = [self createQueueWithName:@"vsp.wrq"
                                    filePath:self.portPath
                                  attributes:DISPATCH_QUEUE_SERIAL];
    
    /* default blocking mode. Reader queue set to
     * non blocking for each read */
    const int fdflags = (O_RDWR | O_NOCTTY);
    const char *_devpath = [self.portPath UTF8String];

    /* monitor serial port */
    if (!self.startMonitoring) {
        
    }
    
    if ((self.fdDevice = open(_devpath, fdflags)) == -1) {
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
    
#ifdef DEBUG
    removeFile(@"received.txt");
#endif
    
    [self configureSerialPort:self.fdDevice];
    [self updateState:SerialPortStateConnected];
    [self monitorPinoutSignals];
    [self startReading];
    
    return YES;
}

- (void)disconnect {
    [self updateState:SerialPortStateDisconnecting];
    
    [self.portAccessLock lock];
    {
        // Remove IOKit notifications
        if (self.notificationPort != NULL) {
            IONotificationPortDestroy(self.notificationPort);
            self.notificationPort = NULL;
        }
        
        if (self.fdDevice != -1) {
            close(self.fdDevice);
            self.fdDevice = -1;
        }
    }
    [self.portAccessLock unlock];

    [self updateState:SerialPortStateDisconnected];
}

- (void)sendFileAtPath:(NSString *)filePath
            chunkSize:(NSUInteger)chunkSize
           completion:(void (^)(BOOL success, NSError *error))completion {

    if (!self.writeQueue) {
        NSError *error = [self createErrorWithCode:EINVAL
                  message:[NSString stringWithUTF8String:strerror(errno)]
                errorType:SerialPortErrorTypeNoQueue];
        [self fireErrorEvent:error withType:SerialPortErrorTypeNoQueue];
        if (completion) {
            completion(NO, error);
        }
        return;
    }

    NSLog(@"Send file %@ chunkSize=%lu", filePath, (unsigned long)chunkSize);

    NSData *fileData = [NSData dataWithContentsOfFile:filePath];
    if (!fileData) {
        NSLog(@"Failed to read file at path: %@", filePath);
        NSError *error = [self createErrorWithCode:EINVAL
                                           message:@"File name required."
                                         errorType:SerialPortErrorTypeNotFound];
        if (completion) {
            completion(NO, error);
        }
        return;
    }
    
    dispatch_async(self.writeQueue, ^{
        const uint8_t *bytes = (const uint8_t *)[fileData bytes];
        NSUInteger totalLength = [fileData length];
        
        if (self.fdDevice == -1 || self.currentState != SerialPortStateConnected) {
            NSError *error = [self createErrorWithCode:ENOTCONN
                                               message:@"Port not connected"
                                             errorType:SerialPortErrorTypeWriteFailed];
            [self fireErrorEvent:error withType:SerialPortErrorTypeWriteFailed];
            if (completion) {
                completion(NO, error);
            }
            return;
        }
                  
        for (NSUInteger i = 0; i < totalLength; i += chunkSize) {
            if (self.fdDevice < 0 || self.currentState != SerialPortStateConnected) {
                break;
            }

            NSUInteger length = MIN(chunkSize, totalLength - i);
            NSData *data = [NSData dataWithBytes:bytes + i length:length];
            const uint8_t *bytes = [data bytes];
            size_t buflen = [data length];

            // Acquire read lock before accessing fdPort
            [self.portAccessLock lock];

            ssize_t written = write(self.fdDevice, bytes, buflen);

            [self.portAccessLock unlock];
            
            if (written < 0) {
                NSError *error = [self
                            createErrorWithCode:errno
                                        message:[NSString stringWithUTF8String:strerror(errno)]
                                      errorType:SerialPortErrorTypeWriteFailed];
                [self fireErrorEvent:error withType:SerialPortErrorTypeWriteFailed];
                if (completion) {
                    completion(NO, error);
                }
                return;
            }
            
            if (written != (ssize_t)length) {
                NSError *error = [self
                            createErrorWithCode:EBADF
                                        message:@"Partial write occurred"
                                      errorType:SerialPortErrorTypeWriteFailed];
#if 1
                [self fireErrorEvent:error withType:SerialPortErrorTypeWriteFailed];
#else
                NSLog(@"Write error? written=%zd < length=%lu", written, (unsigned long)length);
#endif // 0
                if (completion) {
                    completion(NO, error);
                }
                return;
            }
            
            // delay
            [NSThread sleepForTimeInterval:0.1];
            
        } // for
        if (completion) {
            completion(YES, NULL);
        }
    });
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
      
        // Acquire read lock before accessing fdPort
        [self.portAccessLock lock];
        
        if (self.fdDevice == -1 || self.currentState != SerialPortStateConnected) {
            NSError *error = [self createErrorWithCode:ENOTCONN
                                               message:@"Port not connected"
                                             errorType:SerialPortErrorTypeWriteFailed];
            [self fireErrorEvent:error withType:SerialPortErrorTypeWriteFailed];
            [self.portAccessLock unlock];
            return;
        }
        
        const uint8_t *bytes = [data bytes];
        size_t length = [data length];
        ssize_t written;
        
        if ((written = write(self.fdDevice, bytes, length)) < 0) {
            NSError *error = [self
                        createErrorWithCode:errno
                                    message:[NSString stringWithUTF8String:strerror(errno)]
                                  errorType:SerialPortErrorTypeWriteFailed];
            [self fireErrorEvent:error withType:SerialPortErrorTypeWriteFailed];
            [self.portAccessLock unlock];
            return;
        }
        if (written != (ssize_t)length) {
            NSError *error = [self
                        createErrorWithCode:EBADF
                                    message:@"Partial write occurred"
                                  errorType:SerialPortErrorTypeWriteFailed];
            [self fireErrorEvent:error withType:SerialPortErrorTypeWriteFailed];
            [self.portAccessLock unlock];
            return;
        }

        success = YES;
        [self.portAccessLock unlock];
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
        uint8_t buffer[4096];
        ssize_t bytesRead;
        int flags;
        
        while (self.fdDevice != -1 && self.currentState == SerialPortStateConnected) {
        
            // Acquire read lock before accessing fdPort
            [self.portAccessLock lock];
            flags = fcntl(self.fdDevice, F_GETFL, 0);
            fcntl(self.fdDevice, F_SETFL, flags | O_NONBLOCK);
            
            bytesRead = read(self.fdDevice, buffer, sizeof(buffer));
            
            flags = fcntl(self.fdDevice, F_GETFL, 0);
            fcntl(self.fdDevice, F_SETFL, flags & ~O_NONBLOCK);
            [self.portAccessLock unlock];
            
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
                
#ifdef DEBUG_FILE
                writeReceivedData(data, bytesRead, @"received.txt");
#endif
                
                if ([self.delegate respondsToSelector:@selector(serialPortDidReceiveData:)]) {
                    [self notifyDelegateReceivedData:data];
                }
            }
            
            [NSThread sleepForTimeInterval:0.01];
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

- (void)monitorPinoutSignals {
    dispatch_async(self.pinsigQueue, ^{
        int ret, status;
        while (self.fdDevice != -1 && self.currentState == SerialPortStateConnected) {
            
            // Acquire read lock before accessing fdPort
            [self.portAccessLock lock];
            ret = ioctl(self.fdDevice, TIOCMGET, &status);
            [self.portAccessLock unlock];
            
            if (ret == 0) {
                BOOL DCD = (status & TIOCM_CAR);
                BOOL DTR = (status & TIOCM_DTR);
                BOOL DSR = (status & TIOCM_DSR);
                BOOL RTS = (status & TIOCM_RTS);
                BOOL CTS = (status & TIOCM_CTS);
                BOOL RI = (status & TIOCM_RI);
                if ([self.delegate respondsToSelector:@selector(serialPort:didUpdatePinoutSignals:DTR:DSR:RTS:CTS:RI:)]) {
                    [self.delegate serialPort:self didUpdatePinoutSignals:DCD DTR:DTR DSR:DSR RTS:RTS CTS:CTS RI:RI];
                } else {
                    NSLog(@"No pinout signal delegate. Terminate pinout signal monitor.");
                    break; // stop
                }
            }
            
            [NSThread sleepForTimeInterval:0.05];
        }
    });
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
