// ********************************************************************
// VSPClient - serial port access
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
#ifndef SerialPortManager_h
#define SerialPortManager_h
#import <Foundation/Foundation.h>
#import "SerialPortParameters.h"
#import <IOKit/IOBSD.h>
#import <IOKit/IOTypes.h>
#import <IOKit/IOKitLib.h>
#import <IOKit/serial/IOSerialKeys.h>

NS_ASSUME_NONNULL_BEGIN

// Serial port states
typedef NS_ENUM(NSInteger, SerialPortState) {
    SerialPortStateDisconnected = 0,
    SerialPortStateConnecting,
    SerialPortStateConnected,
    SerialPortStateError,
    SerialPortStateDisconnecting
};

// Error types for serial port operations
typedef NS_ENUM(NSInteger, SerialPortErrorType) {
    SerialPortErrorTypeUnknown = 0,
    SerialPortErrorTypePermissionDenied,
    SerialPortErrorTypeNotFound,
    SerialPortErrorTypeAlreadyOpen,
    SerialPortErrorTypeReadTimeout,
    SerialPortErrorTypeWriteFailed,
    SerialPortErrorTypeConfigurationFailed,
    SerialPortErrorTypeHardwareFailure,
    SerialPortErrorTypeNoQueue
};

@protocol SerialPortDelegate <NSObject>
- (void)serialPortDidReceiveData:(NSData *)data;
- (void)serialPortDidConnect;
- (void)serialPortDidDisconnect:(NSError *)error;
- (void)serialPortStateChanged:(SerialPortState)state;
- (void)serialPortDidError:(NSError *)error withType:(SerialPortErrorType)errorType;
- (void)serialPort:(id)port didChangeBaudRate:(NSUInteger)baudRate;
- (void)serialPort:(id)port didChangeDataBits:(NSUInteger)dataBits;
- (void)serialPort:(id)port didChangeStopBits:(NSUInteger)stopBits;
- (void)serialPort:(id)port didChangeParity:(NSUInteger)parity;
- (void)serialPort:(id)port didChangeFlowControl:(NSUInteger)flowControl;
- (void)serialPort:(id)port didUpdatePinoutSignals:(BOOL)DCD DTR:(BOOL)DTR DSR:(BOOL)DSR RTS:(BOOL)RTS CTS:(BOOL)CTS RI:(BOOL)RI;
@end

@interface SerialPort : NSObject

@property (nonatomic, weak) id<SerialPortDelegate> delegate;
@property (nonatomic, strong) SerialPortParameters *parameters;
@property (nonatomic, readonly) SerialPortState currentState;
@property (nonatomic, readonly) NSString *portPath;
@property (nonatomic, readonly) BOOL isConnected;
/* static */
+ (NSArray<NSString *> *)getAvailableSerialPorts NS_SWIFT_NAME(availableSerialPorts());
/* -- */
- (instancetype)initWithPortPath:(NSString *)portPath;
- (instancetype)initWithPathAndParameters:(NSString *)portPath
                      parameters:(SerialPortParameters*)parameters;

- (void)setParameters:(SerialPortParameters *)newParams NS_SWIFT_NAME(setParameters(_:));
- (BOOL)connect NS_SWIFT_NAME(connect());
- (void)disconnect NS_SWIFT_NAME(disconnect());
- (BOOL)sendData:(NSData *)data NS_SWIFT_NAME(send(_:));
- (void)sendFileAtPath:(NSString *)filePath
            chunkSize:(NSUInteger)chunkSize
           completion:(void (^)(BOOL success, NSError *error))completion;
// State management
- (void)updateState:(SerialPortState)state;
- (NSString *)stateDescription:(SerialPortState)state;

// Error handling
- (NSError *)createErrorWithCode:(NSInteger)code
                           message:(NSString *)message
                          errorType:(SerialPortErrorType)errorType;
- (void)fireErrorEvent:(NSError *)error withType:(SerialPortErrorType)errorType;
- (void)fireConnectError:(NSString *)errorMessage
                    errorType:(SerialPortErrorType)errorType;

@end

NS_ASSUME_NONNULL_END

#endif /* SerialPortManager_h */
