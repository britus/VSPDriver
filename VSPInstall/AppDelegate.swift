//
//  AppDelegate.swift
//  VSPInstall
//
//  Created by Björn Eschrich on 30.01.25.
//

import Cocoa
import SystemExtensions
import AppKit
import SwiftUI

let DRIVER_EXTENSION : String = "org.eof.tools.VSPDriver"

@main
class AppDelegate: NSObject, NSApplicationDelegate {

    @IBOutlet var window: NSWindow!
    @IBOutlet weak var txtStatus: NSTextFieldCell!

    // --
    
    func applicationWillHide(_ notification: Notification) {
        NSApplication.shared.terminate(nil)
    }

    func applicationDidFinishLaunching(_ aNotification: Notification) {
        //
    }

    func applicationWillTerminate(_ aNotification: Notification) {
        // Insert code here to tear down your application
    }

    func applicationSupportsSecureRestorableState(_ app: NSApplication) -> Bool {
        return true
    }
    
    @IBAction func onInstallClick(_ sender: NSButton) {
        let request = OSSystemExtensionRequest.activationRequest(
              forExtensionWithIdentifier: DRIVER_EXTENSION,
              queue: .main)
        request.delegate = self
   
        OSSystemExtensionManager.shared.submitRequest(request)
    }
    
    @IBAction func onTestClick(_ sender: NSButton) {
        //
    }

    @IBAction func onQuitClick(_ sender: Any) {
        NSApplication.shared.terminate(nil)
    }
}

extension AppDelegate: OSSystemExtensionRequestDelegate
{
    func request(_ request: OSSystemExtensionRequest,
                actionForReplacingExtension existing: OSSystemExtensionProperties,
                withExtension ext: OSSystemExtensionProperties) -> OSSystemExtensionRequest.ReplacementAction {
        return .replace
    }

    func requestNeedsUserApproval(_ request: OSSystemExtensionRequest) {
        txtStatus.title = "VSP driver activation: User approval required.\n"
        print(txtStatus.title)
    }

    func request(_ request: OSSystemExtensionRequest, didFinishWithResult result: OSSystemExtensionRequest.Result) {
        txtStatus.title = "VSP driver activation succeeded: \(result)\n"
        print(txtStatus.title)
    }

    func request(_ request: OSSystemExtensionRequest, didFailWithError error: Error) {
        txtStatus.title = "VSP driver activation failed:\n \(error)"
        print(txtStatus.title)
    }
}
