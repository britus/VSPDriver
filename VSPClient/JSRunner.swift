// ********************************************************************
// VSPClient User Interface
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
import Foundation
import JavaScriptCore

// Define the delegate protocol
protocol ScriptExecutionDelegate: AnyObject {
    func scriptExecutionDidStart(_ context: JSContext)
    func scriptExecutionDidFinish(_ context: JSContext)
    func scriptExecutionDidFail(_ message: String)
}

/// Simple JS runner for macOS AppKit projects with a Swift->JS callback
class JSRunner {
    private static let lock = NSLock()
    private let workQueue: DispatchQueue = DispatchQueue(
            label: "vspjs.work.queue", qos: .background)
    public var scriptFile: URL?
    /// Callbacks callable from JS
    var onStart: ((String) -> Void)?
    var onComplete: ((String) -> Void)?
    var onMessage: ((String) -> Void)?
    var onSendText: ((String) -> Void)?

    public var rlTerminate: Bool = false
    public var delegate: ScriptExecutionDelegate? = nil

    init(_ context: JSContext) {
        setupContext(context)
    }
    
    private func setupContext(_ context: JSContext) {
        // Handle JS exceptions
        context.exceptionHandler = { _, exception in
            if let exception = exception {
                let msg = "JS: \(String(describing: exception))"
                //NSLog("JS Exception: \(msg)")
                // Notify about error
                self.delegate?.scriptExecutionDidFail(msg)
            }
        }
        
        // Expose Swift callbacks
        context.setObject(
            unsafeBitCast({ [weak self] (message: String) in
                self?.onStart?(message)
            } as @convention(block) (String) -> Void, to: AnyObject.self),
            forKeyedSubscript: "onStart" as NSString
        )
        
        context.setObject(
            unsafeBitCast({ [weak self] (message: String) in
                self?.onComplete?(message)
            } as @convention(block) (String) -> Void, to: AnyObject.self),
            forKeyedSubscript: "onComplete" as NSString
        )

        context.setObject(
            unsafeBitCast({ [weak self] (message: String) in
                self?.onMessage?(message)
            } as @convention(block) (String) -> Void, to: AnyObject.self),
            forKeyedSubscript: "onMessage" as NSString
        )
        
        context.setObject(
            unsafeBitCast({ [weak self] (message: String) in
                self?.onSendText?(message)
            } as @convention(block) (String) -> Void, to: AnyObject.self),
            forKeyedSubscript: "onSendText" as NSString
        )
    }
    
    func setVarialble(_ context: JSContext, _ name: String, _ data: Data)
    {
        JSRunner.lock.lock()
        let  buffer = Array(data).map { UInt8($0) }
        context.setObject(buffer, forKeyedSubscript: name as NSString)
        JSRunner.lock.unlock()
    }
    
    func setVarialble(_ context: JSContext, _ name: String, _ state: Bool)
    {
        JSRunner.lock.lock()
        context.setObject(state, forKeyedSubscript: name as NSString)
        JSRunner.lock.unlock()
    }

    /// Execute a JavaScript string
    func run(_ context: JSContext, script: String) {
        let code: () -> Void = {
            JSRunner.lock.lock()
            //NSLog("JS: \(String(describing: context.virtualMachine.description))")
            //NSLog("JS: \(String(describing: context.virtualMachine.publisher))")

            // Notify before execution
            self.delegate?.scriptExecutionDidStart(context)
            
            // Execute the script
            context.evaluateScript(script)
            
            // Notify after successful execution
            self.delegate?.scriptExecutionDidFinish(context)
            
            self.complete()
            JSRunner.lock.unlock()
        }
        if DispatchQueue.isOnMainQueue() {
            workQueue.async { code() }
        } else {
            code()
        }
    }
    
    private func complete()
    {
        //NSLog("JS: script complete")
    }
}
