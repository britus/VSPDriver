// ********************************************************************
// VSPTestModel.swift - VSP setup app
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
import Foundation

@_silgen_name("SwiftAsyncCallback")
func SwiftAsyncCallback(refcon: UnsafeMutableRawPointer, result: IOReturn, args: UnsafeMutableRawPointer, numArgs: UInt32) {
    let viewModel: VSPTestModel = Unmanaged<VSPTestModel>.fromOpaque(refcon).takeUnretainedValue()
    let argsPointer = args.bindMemory(to: UInt8.self, capacity: Int(numArgs * 8))
    let argsBuffer = UnsafeBufferPointer(start: argsPointer, count: Int(numArgs * 8))

    viewModel.LocalAsyncCallback(result: result, data: Array(argsBuffer))
}

@_silgen_name("SwiftDeviceAdded")
func SwiftDeviceAdded(refcon: UnsafeMutableRawPointer, connection: io_connect_t) {
    let viewModel: VSPTestModel = Unmanaged<VSPTestModel>.fromOpaque(refcon).takeUnretainedValue()
    viewModel.connection = connection
    viewModel.isConnected = true
}

@_silgen_name("SwiftDeviceRemoved")
func SwiftDeviceRemoved(refcon: UnsafeMutableRawPointer) {
    let viewModel: VSPTestModel = Unmanaged<VSPTestModel>.fromOpaque(refcon).takeUnretainedValue()
    viewModel.connection = 0
    viewModel.isConnected = false
}

class VSPSmController {

    enum State {
        case unknown
        case waiting
        case success
        case noCallback
        case error
    }

    enum Event {
        case sentRequest
        case returned
        case failed
        case foundNoCallback
    }

    static func process(_ state: State, _ event: Event) -> State {
        switch event {
        case .sentRequest:
            return .waiting
        case .returned:
            return .success
        case .failed:
            return .error
        case .foundNoCallback:
            return .noCallback
        }
    }
}

// --------------------------------------------------------
// VSPController async handling
// --------------------------------------------------------
class VSPTestModel: NSObject, ObservableObject {
    
    @Published public var isConnected: Bool = false
    public var connection: io_connect_t = 0
    public var message: String = ""
    // Used by async request/response
    var opaqueSelf: UnsafeMutableRawPointer? = //
    UnsafeMutableRawPointer(bitPattern: 0)
    
    @Published private var state: VSPSmController.State = .unknown
    public var stateDescription: String {
        switch state {
        case .unknown:
            return "Waiting for action"
        case .waiting:
            return "Sent request, waiting for response"
        case .success:
            return "Request returned successfully"
        case .noCallback:
            return "Assign a callback before you send an async message"
        case .error:
            return "Request returned an error, check the logs for details"
        }
    }
    
    override init() {
        super.init()
        
        // Create a reference to this view model so the C code can call back to it.
        self.opaqueSelf = Unmanaged.passRetained(self).toOpaque()
        
        // Let the C code set up the IOKit calls.
        UserClientSetup(opaqueSelf)
    }
    
    convenience init(isConnected: Bool) {
        self.init()
        self.isConnected = isConnected
    }
    
    deinit {
        // Take the last reference of the pointer so it can be freed.
        _ = Unmanaged<VSPTestModel>.fromOpaque(self.opaqueSelf!).takeRetainedValue()
        
        // Let the C code clean up after itself.
        UserClientTeardown()
    }
    
    func LocalAsyncCallback(result: IOReturn, data: [UInt8]) {
        if (result != kIOReturnSuccess) {
            state = VSPSmController.process(state, .failed)
        } else {
            state = VSPSmController.process(state, .returned)
        }
    }
}

// --------------------------------------------------------
// VSPController interface commands
// --------------------------------------------------------
extension VSPTestModel {
    func doGetPortList() {
        state = VSPSmController.process(state, .sentRequest)
        
        if !GetPortList(opaqueSelf, connection) {
            state = VSPSmController.process(state, .failed)
        }
    }
    
    func doLinkPorts() {
        state = VSPSmController.process(state, .sentRequest)
         
        if !LinkPorts(opaqueSelf, connection, 1, 2) {
            state = VSPSmController.process(state, .failed)
        }
    }
    
    func doUnLinkPorts() {
        state = VSPSmController.process(state, .sentRequest)
 
        if !UnlinkPorts(opaqueSelf, connection, 1, 2) {
            state = VSPSmController.process(state, .failed)
        }
    }
    
    func doEnablePortChecks() {
        state = VSPSmController.process(state, .sentRequest)
        
        if !EnableChecks(opaqueSelf, connection, 2) {
            state = VSPSmController.process(state, .failed)
        }
    }
    
    func doEnablePortTrace() {
        state = VSPSmController.process(state, .sentRequest)
        
        if !EnableTrace(opaqueSelf, connection, 3) {
            state = VSPSmController.process(state, .failed)
        }
    }
}
