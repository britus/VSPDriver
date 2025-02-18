// ********************************************************************
// VSPLoaderModel.swift - VSP setup app
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
import Foundation
import SystemExtensions
import os.log

class VSPLoaderModel: NSObject {
    // iOS dexts must have the same initial path as their app,
    // so you can use the app's bundle identifier to build the
    // reference to your dext's bundle identifier. The same
    // convention is used for the macOS app for convenience.
    //private let dextIdentifier: String = Bundle.main.bundleIdentifier! + ".driver"
    private let dextIdentifier: String = "org.eof.tools.VSPDriver"

    // Your dext may not start in unloaded state every time. Add logic or states to check this.
    @Published private var state: VSPSmLoader.State = .unknown
    
    private var installedDextProperties: OSSystemExtensionProperties? = nil
    
    public var dextLoadingState: String {
        switch state {
        case .unknown:
            return "Driver state unknown."
        case .unloaded:
            return "Driver isn't loaded."
        case .activating:
            return "Activating driver, please wait."
        case .needsApproval:
            return "Please follow the prompt to approve the driver extension."
        case .activated:
            return "Driver has been activated and is ready to use."
        case .removal:
            return "Driver has been unloaded."
        case .activationError:
            return "Setup has experienced an error during activation.\nPlease check the logs to find the error."
        }
    }
    
    var isUserUnload : Bool = false
    
    override init() {
        super.init()
        
        // Approximate what SDK is available based on what version of Swift is available
#if swift(>=5.5)
        if #available(macOS 12, *) {
            // Since this request impacts the UI, send it with qos userInteractive
            let request = OSSystemExtensionRequest
                .propertiesRequest(forExtensionWithIdentifier: dextIdentifier,
                                   queue: .global(qos: .userInteractive))
            request.delegate = self
            OSSystemExtensionManager.shared.submitRequest(request)
        }
#endif
    }
}

extension VSPLoaderModel: ObservableObject {
    
}

extension VSPLoaderModel {
    
    func activateMyDext()
    {
        activateExtension(dextIdentifier)
    }
    
    /// - Tag: ActivateExtension
    func activateExtension(_ dextIdentifier: String)
    {
        self.isUserUnload = false
        let request = OSSystemExtensionRequest
            .activationRequest(forExtensionWithIdentifier: dextIdentifier,
                               queue: .main)
        request.delegate = self
        OSSystemExtensionManager.shared.submitRequest(request)
        
        self.state = VSPSmLoader.process(self.state, .activationStarted)
    }
    
    func removeMyDext()
    {
        deactivateExtension(dextIdentifier)
    }
    
    // This method isn't used in this example, but is provided for completeness.
    func deactivateExtension(_ dextIdentifier: String)
    {
        self.isUserUnload = true
        let request = OSSystemExtensionRequest
            .deactivationRequest(forExtensionWithIdentifier: dextIdentifier,
                                 queue: .main)
        request.delegate = self
        OSSystemExtensionManager.shared.submitRequest(request)
        
        self.state = VSPSmLoader.process(self.state, .uninstallStarted)
    }
}

extension VSPLoaderModel: OSSystemExtensionRequestDelegate {
    
    func request(_ request: OSSystemExtensionRequest,
                 actionForReplacingExtension existing: OSSystemExtensionProperties,
                 withExtension ext: OSSystemExtensionProperties) -> OSSystemExtensionRequest.ReplacementAction
    {
        var replacementAction: OSSystemExtensionRequest.ReplacementAction
        
        os_log("sysex actionForReplacingExtension: %@ %@", existing, ext)
        
        // Add appropriate logic here to determine if the extension should be
        // replaced by the new extension. Common things to check for include
        // testing whether the new extension's version number is newer than
        // the current version number, or that the bundleIdentifier has changed.
        // For simplicity, this sample always replaces the current extension
        // with the new one.
        replacementAction = .replace
        
        // The upgrade case may require a separate set of states.
        if isUserUnload {
            self.state = VSPSmLoader.process(self.state, .uninstallStarted)
        } else {
            self.state = VSPSmLoader.process(self.state, .activationStarted)
        }
        
        return replacementAction
    }
    
    func requestNeedsUserApproval(_ request: OSSystemExtensionRequest)
    {
        os_log("sysex requestNeedsUserApproval")
        self.state = VSPSmLoader.process(self.state, .promptForApproval)
    }
    
    func request(_ request: OSSystemExtensionRequest, didFinishWithResult result: OSSystemExtensionRequest.Result)
    {
        os_log("sysex didFinishWithResult: %d", result.rawValue)
        
        // The "result" may be "willCompleteAfterReboot", which would require another state.
        // This sample ignores this state for simplicity, but a production app should check for it.
        if self.isUserUnload {
            self.state = VSPSmLoader.process(self.state, .uninstallFinished)
        } else {
            self.state = VSPSmLoader.process(self.state, .activationFinished)
        }
    }
    
    func request(_ request: OSSystemExtensionRequest, didFailWithError error: Error)
    {
        os_log("sysex didFailWithError: %@", error.localizedDescription)
        
        // Some possible errors to check for:
        // Error 4: The dext identifier string in the code needs to match the
        //          one used in the project settings.
        // Error 8: Indicates a signing problem. During development, set signing
        //          to "automatic" and "sign to run locally". See README.md for more.
        
        // While this app only logs errors, production apps should provide feedback
        // to customers about any errors encountered while loading the dext.
        DispatchQueue.main.async {
            self.state = VSPSmLoader.process(self.state, .activationFailed)
        }
    }
    
#if swift(>=5.5)
    func request(_ request: OSSystemExtensionRequest, foundProperties properties: [OSSystemExtensionProperties])
    {
        if #available(macOS 12, *) {
            
            os_log("sysex foundProperties")
            
            // Discover any previously enabled version of the dext.
            // Prefer installations with a higher version number.
            let sortedProperties = properties.sorted(by: { $0.bundleVersion > $1.bundleVersion })
            for dext in sortedProperties where dext.isEnabled {
                self.installedDextProperties = dext
                break
            }
            
            DispatchQueue.main.async {
                self.state = VSPSmLoader.process(self.state,
                                                 self.installedDextProperties != nil
                                                 ? .discoveredLoaded : .discoveredUnloaded)
            }
        }}
#endif
}

/* Loader state machine */
class VSPSmLoader {
    
    enum State {
        case unknown
        case unloaded
        case activating
        case needsApproval
        case activated
        case activationError
        case removal
    }
    
    enum Event {
        case discoveredUnloaded
        case discoveredLoaded
        case activationStarted
        case uninstallStarted
        case promptForApproval
        case activationFinished
        case activationFailed
        case uninstallFinished
    }
    
    static func process(_ state: State, _ event: Event) -> State {
        
        switch state {
        case .unknown, .unloaded:
            switch event {
                case .discoveredUnloaded:
                    return .unloaded
                case .discoveredLoaded:
                    return .activated
                case .activationStarted:
                    return .activating
                case .promptForApproval, .activationFinished, .activationFailed:
                    return .activationError
                case .uninstallStarted:
                    return .removal
                case .uninstallFinished:
                    return .removal
            }
            
        case .activating, .needsApproval:
            switch event {
                case .activationStarted:
                    return .activating
                case .promptForApproval:
                    return .needsApproval
                case .activationFinished:
                    return .activated
                case .activationFailed, .discoveredUnloaded, .discoveredLoaded:
                    return .activationError
                case .uninstallStarted:
                    return .removal
                case .uninstallFinished:
                    return .removal
            }
            
        case .activated:
            switch event {
                case .activationStarted:
                    return .activating
                case .promptForApproval,
                        .activationFailed,
                        .discoveredUnloaded,
                        .discoveredLoaded:
                    return .activationError
                case .activationFinished:
                    return .activated
                case .uninstallStarted:
                    return .removal
                case .uninstallFinished:
                    return .removal
            }
            
        case .activationError:
            switch event {
                case .activationStarted:
                    return .activating
                case .promptForApproval,
                        .activationFinished,
                        .activationFailed,
                        .uninstallStarted,
                        .uninstallFinished,
                        .discoveredUnloaded,
                        .discoveredLoaded:
                    return .activationError
            }
        case .removal:
            switch event {
                case .uninstallStarted:
                    return .removal
                case .promptForApproval,
                        .activationStarted,
                        .activationFinished,
                        .activationFailed,
                        .uninstallFinished,
                        .discoveredUnloaded,
                        .discoveredLoaded:
                    return .activationError
            }
        }
    }
}
