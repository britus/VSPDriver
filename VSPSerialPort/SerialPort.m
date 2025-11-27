// ********************************************************************
// VSPClient - serial port access
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
#import <Foundation/Foundation.h>
#import <sys/types.h>
#import <sys/stat.h>
#import <string.h>
#import <time.h>
#import <sys/ioctl.h>
#import <termios.h>
#import <fcntl.h>
#import <unistd.h>
#import <errno.h>
#import "SerialPort.h"

@interface SerialPort ()
@property (nonatomic, assign) int fdDevice;
@property (nonatomic, strong) dispatch_queue_t readerQueue;
@property (nonatomic, strong) dispatch_queue_t writerQueue;
@property (nonatomic, strong) dispatch_queue_t pinsigQueue;
@property (nonatomic, strong) NSLock *portAccessLock;
@property (nonatomic, strong) NSMutableData *receivedData;
@property (nonatomic, assign) SerialPortState currentState;
@property (nonatomic, strong) NSString *lastErrorMessage;
@property (nonatomic, assign) io_connect_t serialPortRef;
@property (nonatomic, assign) IONotificationPortRef notificationPort;
@property (nonatomic, assign) CFRunLoopSourceRef runLoopSource;
@property (nonatomic, strong) dispatch_semaphore_t cancellationSemaphore;
@end

#ifdef DEBUG_FILE // For debug purposes only (O.o)
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
        _readerQueue = NULL;
        _writerQueue = NULL;
        _pinsigQueue = NULL;
        _receivedData = [[NSMutableData alloc] init];
        _portAccessLock = [[NSLock alloc] init];
        _portPath = portPath;
        _fdDevice = -1;
        _serialPortRef = 0;
        _notificationPort = MACH_PORT_NULL;
        _runLoopSource = NULL;
        _cancellationSemaphore = dispatch_semaphore_create(0);
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
    
    // Sort the ports array by name
    return [ports sortedArrayUsingComparator:^NSComparisonResult(id obj1, id obj2) {
        return [obj1 compare:obj2];
    }];
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

- (void)handleConnectionError:(NSString *)message withType:(SerialPortErrorType)errorType {
    NSError *error = [self createErrorWithCode:ENODEV message:message errorType:errorType];
    [self fireError:error withType:errorType];
}

- (void)updateState:(SerialPortState)state {
    @synchronized(self) {
        if (self.currentState != state) {
            self.currentState = state;
            [self fireStateChanged:state];
        }
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

// Method to create queues with custom names
- (dispatch_queue_t)createQueue:(NSString *)baseName
                       filePath:(NSString *)filePath
                     attributes:(dispatch_queue_attr_t)attributes {
    
    // Generate unique queue name based on base name and device name
    NSString *devName = [filePath lastPathComponent];
    NSString *queueName = [NSString stringWithFormat:@"%@_%@", baseName, devName];
    const char* name = [queueName UTF8String];
    
    // Create queue with background priority
    dispatch_queue_t queue = dispatch_queue_create(name, attributes);
    
    // Set the queue to run with background priority
    dispatch_queue_t backgroundQueue = dispatch_get_global_queue(QOS_CLASS_UTILITY, 0);
    dispatch_set_target_queue(queue, backgroundQueue);

    return queue;
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

- (BOOL)cancellation {
    // Check for cancellation with timeout (100ms)
    if (dispatch_semaphore_wait(
            self.cancellationSemaphore,
            dispatch_time(DISPATCH_TIME_NOW, 100 * NSEC_PER_MSEC)) == 0)
    {
        // Semaphore was signaled - cancel operation
        // Reset for other threads
        dispatch_semaphore_signal(self.cancellationSemaphore);
        return YES;
    }
    return NO;
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
    
#ifdef DEBUG_FILE
    removeFile(@"received.txt");
#endif
    
    // Reset cancellation semaphore for new connection
    if (self.cancellationSemaphore) {
        while (dispatch_semaphore_wait(
                self.cancellationSemaphore, DISPATCH_TIME_NOW) == 0)
        { /* ... */ }
    }

    // Create queues with port name included
    self.pinsigQueue = [self createQueue:@"vsp.psq"
                                filePath:self.portPath
                              attributes:DISPATCH_QUEUE_CONCURRENT];
    self.readerQueue = [self createQueue:@"vsp.rdq"
                                filePath:self.portPath
                              attributes:DISPATCH_QUEUE_CONCURRENT];
    self.writerQueue = [self createQueue:@"vsp.wrq"
                                filePath:self.portPath
                              attributes:DISPATCH_QUEUE_CONCURRENT];
    
    /* default blocking mode. Reader queue set to
     * non blocking for each read */
    const int fdflags = (O_RDWR | O_NOCTTY);
    const char *_devpath = [self.portPath UTF8String];

    /* monitor serial port */
    if (!self.startMonitoring) { }
    
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
    
    [self configureSerialPort:self.fdDevice];
    [self updateState:SerialPortStateConnected];
    [self monitorPinoutSignals];
    [self startReading];
    
    _isConnected = YES;
    
    return self.isConnected;
}

- (void)disconnect {
    [self updateState:SerialPortStateDisconnecting];
    
    // Signal cancellation
    if (self.cancellationSemaphore) {
        dispatch_semaphore_signal(self.cancellationSemaphore);
    }
    
    // Give background threads a moment to respond
    [NSThread sleepForTimeInterval:0.2];

    // Wait for queues to finish or cancel them
    if (self.readerQueue) {
        dispatch_sync(self.readerQueue, ^{
            // Empty block to ensure reader operations complete
        });
    }    
    if (self.pinsigQueue) {
        dispatch_sync(self.pinsigQueue, ^{
            // Empty block to ensure pin monitoring completes
        });
    }
    if (self.writerQueue) {
        dispatch_sync(self.readerQueue, ^{
            // Empty block to ensure reader operations complete
        });
    }

    [self.portAccessLock lock];
    {
        // Signal background queues to stop first
        self.currentState = SerialPortStateDisconnecting;
        
        // internally switch off
        _isConnected = NO;

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

    // Reset semaphore for future connections
    if (self.cancellationSemaphore) {
        // Drain any remaining signals (Consume all signals)
        while (dispatch_semaphore_wait(
               self.cancellationSemaphore, DISPATCH_TIME_NOW) == 0)
        { /* ... */ }
    }

    [self updateState:SerialPortStateDisconnected];
}

- (void)sendFileAtPath:(NSString *)filePath
            chunkSize:(NSUInteger)chunkSize
           completion:(void (^)(BOOL success, NSError *error))completion {

    if (!self.writerQueue) {
        NSError *error = [self createErrorWithCode:EINVAL
                  message:[NSString stringWithUTF8String:strerror(errno)]
                errorType:SerialPortErrorTypeNoQueue];
        [self fireError:error withType:SerialPortErrorTypeNoQueue];
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
    
    dispatch_async(self.writerQueue, ^{
        const uint8_t *bytes = (const uint8_t *)[fileData bytes];
        NSUInteger totalLength = [fileData length];
        
        if (self.fdDevice == -1 || self.currentState != SerialPortStateConnected) {
            NSError *error = [self createErrorWithCode:ENOTCONN
                                               message:@"Port not connected"
                                             errorType:SerialPortErrorTypeWriteFailed];
            [self fireError:error withType:SerialPortErrorTypeWriteFailed];
            if (completion) {
                completion(NO, error);
            }
            return;
        }
                  
        for (NSUInteger i = 0; i < totalLength; i += chunkSize) {
      
            // Check for cancellation with timeout
            if (self.cancellation) {
                break;
            }

            // Acquire read lock before accessing fdDevice
            [self.portAccessLock lock];
        
            if (self.fdDevice < 0 || self.currentState != SerialPortStateConnected) {
                [self.portAccessLock unlock];
                break;
            }
            
            NSUInteger length = MIN(chunkSize, totalLength - i);
            NSData *data = [NSData dataWithBytes:bytes + i length:length];
            const uint8_t *bytes = [data bytes];
            size_t buflen = [data length];
            ssize_t written = write(self.fdDevice, bytes, buflen);
            
            [self.portAccessLock unlock];
            
            if (written < 0) {
                NSError *error = [self
                    createErrorWithCode:errno
                                message:[NSString stringWithUTF8String:strerror(errno)]
                              errorType:SerialPortErrorTypeWriteFailed];
                [self fireError:error withType:SerialPortErrorTypeWriteFailed];
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
                [self fireError:error withType:SerialPortErrorTypeWriteFailed];
                if (completion) {
                    completion(NO, error);
                }
                return;
            }
        } // for
        
        if (completion) {
            completion(YES, NULL);
        }
    });
}


- (BOOL)sendData:(NSData *)data {
    __block BOOL success = NO;
    
    if (!self.writerQueue) {
        NSError *error = [self createErrorWithCode:EINVAL
                  message:[NSString stringWithUTF8String:strerror(errno)]
                errorType:SerialPortErrorTypeNoQueue];
        [self fireError:error withType:SerialPortErrorTypeNoQueue];
        return NO;
    }
   
    dispatch_sync(self.writerQueue, ^{
      
        // Acquire read lock before accessing fdDevice
        [self.portAccessLock lock];
        {
            if (self.fdDevice == -1 || self.currentState != SerialPortStateConnected) {
                NSError *error = [self createErrorWithCode:ENOTCONN
                                                   message:@"Port not connected"
                                                 errorType:SerialPortErrorTypeWriteFailed];
                [self fireError:error withType:SerialPortErrorTypeWriteFailed];
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
                [self fireError:error withType:SerialPortErrorTypeWriteFailed];
                [self.portAccessLock unlock];
                return;
            }
            if (written != (ssize_t)length) {
                NSError *error = [self
                                  createErrorWithCode:EBADF
                                  message:@"Partial write occurred"
                                  errorType:SerialPortErrorTypeWriteFailed];
                [self fireError:error withType:SerialPortErrorTypeWriteFailed];
                [self.portAccessLock unlock];
                return;
            }
            
            success = YES;
        }
        [self.portAccessLock unlock];
    });
      
    return success;
}

- (void)startReading {
    if (!self.readerQueue) {
        NSError *error = [self createErrorWithCode:EINVAL
                  message:[NSString stringWithUTF8String:strerror(errno)]
                errorType:SerialPortErrorTypeNoQueue];
        [self fireError:error withType:SerialPortErrorTypeNoQueue];
        return;
    }
    dispatch_async(self.readerQueue, ^{
        BOOL shouldContinue;
        uint8_t buffer[4096];
        ssize_t bytesRead;
        int flags;
        
        while (YES) {
            
            // Check for cancellation with timeout
            if (self.cancellation) {
                break;
            }
            
            // Acquire read lock before accessing fdDevice
            [self.portAccessLock lock];
            {
                shouldContinue = (self.fdDevice != -1 &&
                                  self.currentState == SerialPortStateConnected &&
                                  self.isConnected);
                if (!shouldContinue) {
                    [self.portAccessLock unlock];
                    break;
                }
                
                flags = fcntl(self.fdDevice, F_GETFL, 0);
                fcntl(self.fdDevice, F_SETFL, flags | O_NONBLOCK);
                
                bytesRead = read(self.fdDevice, buffer, sizeof(buffer));
                
                flags = fcntl(self.fdDevice, F_GETFL, 0);
                fcntl(self.fdDevice, F_SETFL, flags & ~O_NONBLOCK);
            }
            [self.portAccessLock unlock];
            
            if (bytesRead < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                } else {
                    NSError *error = [self createErrorWithCode:errno
                               message:[NSString stringWithUTF8String:strerror(errno)]
                             errorType:SerialPortErrorTypeReadTimeout];
                    [self fireError:error withType:SerialPortErrorTypeReadTimeout];
                    break;
                }
            }
            // No data available, continue
            else if (bytesRead == 0) {
                continue;
            }
            // Data received
            else if (bytesRead > 0) {
                NSData *data = [NSData dataWithBytes:buffer length:(size_t)bytesRead];
#ifdef DEBUG_FILE
                writeReceivedData(data, bytesRead, @"received.txt");
#endif
                if ([self.delegate respondsToSelector:@selector(serialPortDidReceiveData:)]) {
                    [self fireReceivedData:data];
                }
            }
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
    if (!self.pinsigQueue) {
        NSError *error = [self createErrorWithCode:EINVAL
                  message:[NSString stringWithUTF8String:strerror(errno)]
                errorType:SerialPortErrorTypeNoQueue];
        [self fireError:error withType:SerialPortErrorTypeNoQueue];
        return;
    }
    dispatch_async(self.pinsigQueue, ^{
        int ret, status;
        BOOL shouldContinue;
        
        while (YES) {
            
            // Check for cancellation with timeout
            if (self.cancellation) {
                break;
            }
            
            // Acquire read lock before accessing fdPort
            [self.portAccessLock lock];
            {
                shouldContinue = (self.fdDevice != -1 &&
                                  self.currentState == SerialPortStateConnected &&
                                  self.isConnected);
                if (!shouldContinue) {
                    [self.portAccessLock unlock];
                    break;
                }
                ret = ioctl(self.fdDevice, TIOCMGET, &status);
            }
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
        }
    });
}

// Notification methods
- (void)fireStateChanged:(SerialPortState)state {
    if ([self.delegate respondsToSelector:@selector(serialPortStateChanged:)]) {
        [self.delegate serialPortStateChanged:state];
    }
    switch (state) {
        case SerialPortStateConnected:
            [self fireConnected];
            break;
        case SerialPortStateDisconnected:
            [self fireDisconnected];
            break;
        default:
            break;
    }
}

- (void)fireReceivedData:(NSData *)data {
    if ([self.delegate respondsToSelector:@selector(serialPortDidReceiveData:)]) {
        [self.delegate serialPortDidReceiveData:data];
    }
}

- (void)fireConnected {
    if ([self.delegate respondsToSelector:@selector(serialPortDidConnect)]) {
        [self.delegate serialPortDidConnect];
    }
}

- (void)fireDisconnected {
    if ([self.delegate respondsToSelector:@selector(serialPortDidDisconnect)]) {
        [self.delegate serialPortDidDisconnect];
    }
}

- (void)fireError:(NSError *)error withType:(SerialPortErrorType)errorType {
    if ([self.delegate respondsToSelector:@selector(serialPortDidError:withType:)]) {
        [self.delegate serialPortDidError:error withType:errorType];
    }
    // [self disconnect];
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
    if ([self.delegate respondsToSelector:@selector(serialPortDidError:withType:)]) {
        [self.delegate serialPortDidError:error withType:errorType];
    }
}

@end
