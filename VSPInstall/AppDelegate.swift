//
//  AppDelegate.swift
//  VSPInstall
//
//  Created by Björn Eschrich on 30.01.25.
//

import Cocoa
import SystemExtensions

@main
class AppDelegate: NSObject, NSApplicationDelegate, OSSystemExtensionRequestDelegate {

    @IBOutlet var window: NSWindow!

    func applicationDidFinishLaunching(_ aNotification: Notification) {
        let request = OSSystemExtensionRequest.activationRequest(
              forExtensionWithIdentifier: "com.example.VirtualSerialPortDriver",
              queue: .main
        )
        request.delegate = self
        OSSystemExtensionManager.shared.submitRequest(request)
    }

    func applicationWillTerminate(_ aNotification: Notification) {
        // Insert code here to tear down your application
    }

    func applicationSupportsSecureRestorableState(_ app: NSApplication) -> Bool {
        return true
    }
    
    func request(_ request: OSSystemExtensionRequest, actionForReplacingExtension existing: OSSystemExtensionProperties, withExtension ext: OSSystemExtensionProperties) -> OSSystemExtensionRequest.ReplacementAction {
      return .replace
    }

    func requestNeedsUserApproval(_ request: OSSystemExtensionRequest) {
      print("User approval required.")
    }

    func request(_ request: OSSystemExtensionRequest, didFinishWithResult result: OSSystemExtensionRequest.Result) {
      print("Activation succeeded.")
      NSApplication.shared.terminate(nil)
    }

    func request(_ request: OSSystemExtensionRequest, didFailWithError error: Error) {
      print("Activation failed: \(error)")
      NSApplication.shared.terminate(nil)
    }
}

