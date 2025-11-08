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
    private let context: JSContext
    public var scriptFile: URL?
    
    /// Callbacks callable from JS
    var onStart: ((String) -> Void)?
    var onComplete: ((String) -> Void)?
    var onMessage: ((String) -> Void)?
    var onSendText: ((String) -> Void)?

    public var rlTerminate: Bool = false
    public var delegate: ScriptExecutionDelegate? = nil

    init() {
        context = JSContext()
        setupContext()
    }
    
    private func setupContext() {
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
    
    func setVarialble(_ name: String, _ data: Data)
    {
        let  buffer = Array(data).map { UInt8($0) }
        context.setObject(buffer, forKeyedSubscript: name as NSString)
    }
    
    func setVarialble(_ name: String, _ state: Bool)
    {
        context.setObject(state, forKeyedSubscript: name as NSString)
    }

    /// Execute a JavaScript string
    func run(script: String) {
        DispatchQueue.global(qos: .background).async {
            // Notify before execution
            self.delegate?.scriptExecutionDidStart(self.context)
            
            // Execute the script
            self.context.evaluateScript(script)
            
            // Notify after successful execution
            self.delegate?.scriptExecutionDidFinish(self.context)
        }
    }
}
