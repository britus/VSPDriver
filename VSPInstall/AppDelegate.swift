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
import IOKit

let DRIVER_EXTENSION : String = "org.eof.tools.VSPDriver"

@main
class AppDelegate: NSObject, NSApplicationDelegate {

    @IBOutlet var window: NSWindow!
    @IBOutlet weak var txtStatus: NSTextFieldCell!

    // --

    func applicationDidFinishLaunching(_ aNotification: Notification) {
        //
    }

    func applicationWillHide(_ notification: Notification) {
        NSApplication.shared.terminate(nil)
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
        checkDriverInstall()
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

extension AppDelegate
{
    func connectToDriver() -> io_connect_t? {
        
        // Match IOClass name from Info.plist
        //let serviceName = "IOUserSerial"
        let serviceName = "VSPDriver"
        
        guard let matchingDict = IOServiceMatching(serviceName) else
        {
            return nil
        }
        
        var iterator: io_iterator_t = 0
        
        let result = IOServiceGetMatchingServices(kIOMainPortDefault, matchingDict, &iterator)
        guard result == KERN_SUCCESS, iterator != 0 else
        {
            return nil
        }
        
        defer { IOObjectRelease(iterator) }
        
        let service = IOIteratorNext(iterator)
        guard service != 0 else
        {
            return nil
        }
        
        defer { IOObjectRelease(service) }
        
        var connect: io_connect_t = 0
        
        let openResult = IOServiceOpen(service, mach_task_self_, 0, &connect)
        guard openResult == KERN_SUCCESS else
        {
            return nil
        }
        
        return connect
    }

    func checkDriverInstall() {
        guard let connect = connectToDriver() else {
            txtStatus.title = "Driver not found"
            return
        }
        
        var input: UInt64 = 123 // Example input
        var output: UInt64 = 0
        var outputCount: Int = 1

        let result = IOConnectCallMethod(
            connect,
            0,                      // Selector index matching ExternalMethod cases
            &input, 1,              // Input values and count
            nil, 0,                 // Input struct (unused here)
            nil, nil,               // Output values and count (unused)
            &output, &outputCount   // Output struct
        )
        
        if result == KERN_SUCCESS {
            txtStatus.title = "Received output: \(output)"
        } else {
            txtStatus.title = "Error: \(result)"
        }

        IOServiceClose(connect)
    }
}
