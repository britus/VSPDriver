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
import ORSSerial

let DRIVER_EXTENSION : String = "org.eof.tools.VSPDriver"

@main
class AppDelegate: NSObject, NSApplicationDelegate {

    @IBOutlet var window: NSWindow!
    @IBOutlet weak var txtStatus: NSTextFieldCell!
    @IBOutlet weak var btnInstall: NSButton!
    @IBOutlet weak var btnTest: NSButton!
    @IBOutlet weak var cbxDevices: NSComboBox!

    // --

    func applicationDidFinishLaunching(_ aNotification: Notification) {
        btnTest.isEnabled = false
        cbxDevices.isEnabled = false
        checkDevicePaths()
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
        NSApplication.shared.terminate(nil)
    }

    func request(_ request: OSSystemExtensionRequest, didFailWithError error: Error) {
        txtStatus.title = "VSP driver activation failed:\n\(error.localizedDescription)\n"
        print(txtStatus.title)
    }
}

extension AppDelegate: ORSSerialPortDelegate
{
    func checkDevicePaths() {
        guard let pathUrl = URL(string: "/dev") else {
            return;
        }
        do {
            // Get the directory contents urls (including subfolders urls)
            let directoryContents = try FileManager.default.contentsOfDirectory(at: pathUrl,
                                                                                includingPropertiesForKeys: nil)
            let map = directoryContents.map {
                $0.absoluteString
            }
            
            self.cbxDevices.removeAllItems();
            
            for url in map {
                let file = url.replacingOccurrences(of: "file://", with: "")
                if file.contains("cu.serial-") {
                    cbxDevices.addItem(withObjectValue: file);
                }
                else if file.contains("tty.serial-") {
                    cbxDevices.addItem(withObjectValue: file);
                }
            }

            cbxDevices.isEnabled = cbxDevices.numberOfItems > 0
            btnTest.isEnabled = cbxDevices.isEnabled
            if cbxDevices.isEnabled {
                cbxDevices.selectItem(at: 0)
            }
        } catch {
            txtStatus.title = "\(error)";
        }
    }
    
    func checkDriverInstall() {
        let dataToSend = "Hello".data(using: .utf8)
        let index = cbxDevices.indexOfSelectedItem
        let path = cbxDevices.itemObjectValue(at: index)
        
        guard let dev = path as! String? else {
            txtStatus.title += "[VSP-Test]: Can't find path: \(path)"
            return
        }
        
        guard var port = ORSSerialPort(path: dev) else {
            txtStatus.title += "[VSP-Test]: Can't open device: \(dev)"
            return
        }
        
        port.delegate = self
        port.baudRate = 9600
        port.parity = .none
        port.numberOfStopBits = 1
        port.usesRTSCTSFlowControl = true
        
        guard let data = dataToSend else {
            txtStatus.title += "[VSP-Test]: booo \(String(describing: dataToSend))"
            return
            
        }
        
        port.send(data)
        
        txtStatus.title += "[VSP-Test]: SUCCESS\n"
    }
    
    func serialPort(_ serialPort: ORSSerialPort, didReceive data: Data) {
        let string = String(data: data, encoding: .utf8)
        txtStatus.title += "[VSP-Test]: Got answer\n\(String(describing: string))"
    }
    
    func serialPortWasClosed(_ serialPort: ORSSerialPort) {
        txtStatus.title += "[VSP-Test]: port closed"
    }
    func serialPortWasOpened(_ serialPort: ORSSerialPort) {
        txtStatus.title += "[VSP-Test]: port opened"
    }
    func serialPort(_ serialPort: ORSSerialPort, didEncounterError error: Error) {
        txtStatus.title += "[VSP-Test]: port error\n\(String(describing: error.localizedDescription))"
    }
    func serialPort(_ serialPort: ORSSerialPort, didReceiveResponse responseData: Data, to request: ORSSerialRequest) {
        txtStatus.title += "[VSP-Test]: \(responseData)"
    }
    func serialPort(_ serialPort: ORSSerialPort, requestDidTimeout request: ORSSerialRequest) {
        txtStatus.title += "[VSP-Test]: request timed out."
    }
    func serialPortWasRemovedFromSystem(_ serialPort: ORSSerialPort) {
        txtStatus.title += "[VSP-Test]: port removed."
    }
    func serialPort(_ serialPort: ORSSerialPort, didReceivePacket packetData: Data, matching descriptor: ORSSerialPacketDescriptor) {
        txtStatus.title += "[VSP-Test]: \(packetData)"
    }
}
