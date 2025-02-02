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
import IOKit.usb
import IOKit.serial

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
        txtStatus.title = "VSP driver activation succeeded:\n\(result)\n"
        print(txtStatus.title)
    }

    func request(_ request: OSSystemExtensionRequest, didFailWithError error: Error) {
        txtStatus.title = "VSP driver activation failed:\n\(error.localizedDescription)\n"
        print(txtStatus.title)
    }
}

extension AppDelegate
{
    // Define configuration struct (matching C layout)
    struct SerialConfig {
        var baudRate: UInt32
        var dataBits: UInt8
        var parity: UInt8
        var stopBits: UInt8
    }
    
    func connectToDriver() -> io_connect_t? {
        // Match the IOUserSerial service
        //let matchingDict = IOServiceMatching(kIOSerialBSDServiceValue)
        let matchingDict = IOServiceMatching("IOUserSerial")

        NSLog("[VSP-Test]: MatchDict: \(String(describing: matchingDict))")

        // --
        var iterator: io_iterator_t = IO_OBJECT_NULL
        let result = IOServiceGetMatchingServices(kIOMainPortDefault, matchingDict, &iterator)
        guard result == kIOReturnSuccess, iterator != IO_OBJECT_NULL else { return nil }
        defer { IOObjectRelease(iterator) }
        
        // --
        var service: io_service_t = IO_OBJECT_NULL
        while case let device = IOIteratorNext(iterator), device != IO_OBJECT_NULL {
            var state: Int = 0;
            if let path = IORegistryEntryCreateCFProperty(device, kIOCalloutDeviceKey as CFString, kCFAllocatorDefault, 0) {
                guard let path = path.takeRetainedValue() as? String else {
                    return nil
                }
                NSLog("[VSP-Test]: kIOCalloutDeviceKey -> \(path)")
                state += 1;
            }
            if let path = IORegistryEntryCreateCFProperty(device, kIODialinDeviceKey as CFString, kCFAllocatorDefault, 0) {
                guard let path = path.takeRetainedValue() as? String else {
                    return nil
                }
                NSLog("[VSP-Test]: kIODialinDeviceKey -> \(path)")
                state += 1;
            }
            //if state != 0 {
                service = device
                break;
            //}
        }
        guard service != IO_OBJECT_NULL else { return nil }
        defer { IOObjectRelease(service) }
        
        // --
        var connect: io_connect_t = IO_OBJECT_NULL
        let openResult = IOServiceOpen(service, mach_task_self_, 0, &connect)
        return openResult == kIOReturnSuccess ? connect : nil
    }

    func connectToDriver2() -> io_connect_t? {
        let matchingDict = IOServiceMatching(kIOSerialBSDServiceValue)
        var iterator: io_iterator_t = 0
        
        guard IOServiceGetMatchingServices(kIOMainPortDefault, matchingDict, &iterator) == KERN_SUCCESS else {
            return nil
        }
        
        defer { IOObjectRelease(iterator) }
        
        while case let device = IOIteratorNext(iterator), device != IO_OBJECT_NULL {
            if let path = IORegistryEntryCreateCFProperty(device, kIOCalloutDeviceKey as CFString, kCFAllocatorDefault, 0) {
                guard let path = path.takeRetainedValue() as? String else {
                    return nil
                }
                NSLog("[VSP-Test]: kIOCalloutDeviceKey -> \(path)")
            }
            if let path = IORegistryEntryCreateCFProperty(device, kIODialinDeviceKey as CFString, kCFAllocatorDefault, 0) {
                guard let path = path.takeRetainedValue() as? String else {
                    return nil
                }
                NSLog("[VSP-Test]: kIODialinDeviceKey -> \(path)")
            }
            
            let fd = IOServiceOpenAsFileDescriptor(device, O_RDWR);
            if (fd != 0) {
                NSLog("[VSP-Test]: service fd -> \(fd)")
            }

            // --
            var connect: io_connect_t = IO_OBJECT_NULL
            let openResult = IOServiceOpen(device, mach_task_self_, 0, &connect)
            
            return openResult == kIOReturnSuccess ? connect : nil
        }
        
        return nil
    }

    func configurePort(_ connect: io_connect_t) -> Bool {
        let config = SerialConfig(
            baudRate: 9600,
            dataBits: 8,
            parity: 0, // 0 = no parity
            stopBits: 1
        )
        
        if (config.baudRate != 0) {
            
        }
        
        return true
    }

    func writeTestData(_ connect: io_connect_t) -> Bool {
        let dataToSend = "Hello, Serial!".data(using: .utf8)!
        var ok = false

        ok = dataToSend.withUnsafeBytes { ptr in
            var result : IOReturn  = kIOReturnSuccess
            
            // -- //
            
            txtStatus.title = "[VSP-Test]: Write result: \(result)\n"
            ok = (result == kIOReturnSuccess)
            return ok;
        }

        return ok
    }
    
    func readTestData(_ connect: io_connect_t) -> Bool {
        var readBuffer = [UInt8](repeating: 0, count: 4096)
        let bytesRead: Int = 0
                
        let data = Data(bytes: &readBuffer, count: bytesRead)
        txtStatus.title = "[VSP-Test]: Received: \(bytesRead) bytes \(String(data: data, encoding: .utf8) ?? "")\n"
        return true
    }
    
    func checkDriverInstall() {
        guard let connect = connectToDriver() else {
            txtStatus.title += "[VSP-Test]: Driver not found!\n"
            return
        }
        
        if configurePort(connect) == false {
            txtStatus.title += "[VSP-Test]: Unable to configure serial port!\n"
            IOServiceClose(connect)
            return
        }
        
        if writeTestData(connect) == false {
            txtStatus.title += "[VSP-Test]: Unable to write serial port.\n"
            IOServiceClose(connect)
            return
        }
        
        if readTestData(connect) == false {
            txtStatus.title += "[VSP-Test]: Unable to read serial port.\n"
            IOServiceClose(connect)
            return
        }

        txtStatus.title += "[VSP-Test]: SUCCESS.\n"
        IOServiceClose(connect)
    }
}
