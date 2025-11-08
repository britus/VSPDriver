// ********************************************************************
// VSPClient User Interface
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
import Cocoa
import AppKit
import Foundation
import Darwin
import JavaScriptCore

class SPTestViewController: NSViewController, SerialPortDelegate, ScriptExecutionDelegate, NSTextFieldDelegate {
    
    @IBOutlet weak var cbxDevice: ComboBox!
    @IBOutlet weak var cbxBaudRate: ComboBox!
    @IBOutlet weak var cbxDataBits: ComboBox!
    @IBOutlet weak var cbxStopBits: ComboBox!
    @IBOutlet weak var cbxParity: ComboBox!
    @IBOutlet weak var cbxFlowCtrl: ComboBox!
    @IBOutlet weak var pbPortOpen: NSButton!
    @IBOutlet weak var pbPortClose: NSButton!
    @IBOutlet weak var gbxSerialTest: NSBox!
    @IBOutlet weak var pbIoLooper: NSButton!
    @IBOutlet weak var pbIoSendText: NSButton!
    @IBOutlet weak var pbIoSendFile: NSButton!
    @IBOutlet weak var edTextField: NSTextField!
    @IBOutlet weak var txLogView: NSTextView!
    @IBOutlet weak var edAutoTextLen: NSTextField!
    @IBOutlet weak var cbxSendTextAddCR: NSButton!
    @IBOutlet weak var cbxSendTextAddLF: NSButton!
    @IBOutlet weak var pbRunScript: NSButton!
    
    struct SerialPortPinouts {
        var DCD: Bool = false
        var DTR: Bool = false
        var DSR: Bool = false
        var RTS: Bool = false
        var CTS: Bool = false
        var  RI: Bool = false
    }
    
    private var serialPort: SerialPort? = nil
    private var portConfig: SerialPortParameters = SerialPortParameters()
    private var serialPins: SerialPortPinouts = SerialPortPinouts()
    private var isLooperRunning: Bool = false
    private var deviceName: String = ""
    private var textToSend: String = ""
    private var autoTextLen: UInt32 = 16
    private var isAddCrEnabled: Bool = true
    private var isAddLfEnabled: Bool = true
    private let jsRunner: JSRunner = JSRunner()
    
    override func viewDidLoad() {
        super.viewDidLoad()
        pbIoLooper.isEnabled = false
        pbRunScript.isEnabled = true
        pbIoSendFile.isEnabled = false
        pbIoSendText.isEnabled = false
        pbPortOpen.isEnabled = false
        pbPortClose.isEnabled = false
        edTextField.isEnabled = false
        edTextField.delegate = self
        edAutoTextLen.isEnabled = false
        cbxSendTextAddCR.isEnabled = false
        cbxSendTextAddCR.state = .on
        cbxSendTextAddLF.isEnabled = false
        cbxSendTextAddLF.state = .on
        txLogView.string = ""
        txLogView.setLineWrapping(false)
        jsRunner.delegate = self
        
        let formatter = NumberFormatter()
        formatter.numberStyle = .none               // or .currency, .percent, etc.
        formatter.usesGroupingSeparator = false     // ← disables thousands separators
        formatter.maximumFractionDigits = 0         // optional, for precision control
        formatter.minimumFractionDigits = 0
        edAutoTextLen.formatter = formatter

        // find available serial port devices (IOReg access)
        populateSerialPorts()
        
        // serial port parameters
        populateComboBoxes()
        
        // defaults
        portConfig.baudRate = 115200
        portConfig.dataBits = 8
        portConfig.stopBits = 1
        portConfig.parity = 0
        portConfig.flowCtrl = 0
    }
    
    private func populateComboBoxes() {
        UITools.populateBaudRateComboBox(cbxBaudRate)
        UITools.populateDataBitsComboBox(cbxDataBits)
        UITools.populateStopBitsComboBox(cbxStopBits)
        UITools.populateParityComboBox(cbxParity)
        UITools.populateFlowControlComboBox(cbxFlowCtrl)
    }

    private func populateSerialPorts() {
        cbxDevice.removeAllItems()
        let ports = SerialPort.availableSerialPorts()
        if !ports.isEmpty {
            ports.forEach {
                cbxDevice.addItem(withText: $0, dataObject: $0)
            }
        }
        cbxDevice.isEnabled = (cbxDevice.numberOfItems > 0)
        if (cbxDevice.isEnabled) {
            cbxDevice.selectItem(at: 0)
            onSerialPortChanged(cbxDevice)
        }
    }
    
    private func notifyPinsChanged(_ pins: SerialPortPinouts)
    {
        let text = "DTR:\(pins.DTR) DSR:\(pins.DSR) RTS:\(pins.RTS) CTS:\(pins.CTS) RI:\(pins.RI)"
        logMessage("\(text)")
    }

    private func toHexLog(_ data: Data) -> String {
        if let text = String(data: data, encoding: .utf8) {
            return "<: \(text)"
        }
        
        let bytes = Array(data)
        
        // Format hex bytes
        // (30 32 33 34 35 36 37 38 39 71 07 0D)
        let hexPart = bytes.map {
            String(format: "%02X", $0)
        }.joined(separator: " ")
        
        // Format with proper spacing
        return hexPart;
    }

    private func logMessage(_ msg: String)
    {
        DispatchQueue.main.async {
            var _text = msg.trimmingCharacters(in: CharacterSet(charactersIn: "\r\n"))
            if !_text.endsWith("\n") {
                _text.append("\n")
            }
            if let stg = self.txLogView.textStorage {
                if stg.length < 32394 {
                    let _buffer = (stg.string as NSString).appending(_text)
                    self.txLogView.setText(_buffer)
                } else {
                    self.txLogView.setText(_text)
                }
                //self.txLogView.scrollToTextEnd()
                self.txLogView.scrollToTextViewEnd()
            }
        }
    }
    
    private func connectToSerialPort() {
        if serialPort != nil {
            serialPort?.disconnect()
            serialPort?.delegate = nil
            serialPort = nil
        }

        guard !deviceName.isEmpty else {
            UITools.showMessage(message: "Please select a serial device first.")
            return
        }

        // connect serial port (character device at FS)
        serialPort = SerialPort(portPath: self.deviceName)
        serialPort?.delegate = self
        serialPort?.parameters = portConfig
        
        guard ((serialPort?.connect()) != nil) else {
            logMessage("Failed to connect serial port.")
            return
        }
    }
    
    func scriptExecutionDidStart(_ context: JSContext) {
        //
    }
    
    func scriptExecutionDidFinish(_ context: JSContext) {
        //
    }
    
    func scriptExecutionDidFail(_ message: String) {
        self.logMessage(message)
    }
    
    @objc func onNewScript() {
        var fileURL: URL?
        // Create JavaScript file in App Home directory
        if let homeDir = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first {
            fileURL = homeDir.appendingPathComponent("newScript.js")
        }
        else if let documentsDirectory = FileManager.default.urls(for: .userDirectory, in: .userDomainMask).first {
            fileURL = documentsDirectory.appendingPathComponent("newScript.js")
        }
        else {
            UITools.showMessage(message: "Unable to create VSP script template.")
            return;
        }

        // JavaScript template content
        let jsTemplate = """
        // VSP Test Script (JS)

        function main() {
            // notify VSP App
            onStart("JS: Start - Hello");
        
            // Displays the value of the received global
            // property "receivedData". The value type 
            // is a byte array. [UInt8]
            if (dataAvailable === true) {
                onMessage("JS: Received data: " + receivedData);
            }
        
            // --
            // TODO: Do something here
            // --
            
            // Send text to linked port.
            onSendText("JS: device command / AT Command");
            
            // notify VSP App
            onComplete("JS: finished.");
        }
        
        // --[Main]--
        main();
        """
        
        // Write template to file
        do {
            guard let url = fileURL else {
                UITools.showMessage(message: "Invalid script URL.")
                return
            }
            try jsTemplate.write(to: url, atomically: true, encoding: .utf8)
            //print("New script created at: $fileURL.path)")
            // open the file in editor or show in Finder
            NSWorkspace.shared.open(url)
        } catch {
            UITools.showMessage(message: "Error creating script file: \(error)")
        }
    }
    
    @objc func onStartScripting() {
        // check current port selection. Port must be
        // linked othewise loopback raise condition
        let model = DataModel.shared
        let count = model.recordCount()
        var found : Bool = false
        for i in 0..<count {
            guard let r = model.getRecord(index: i, byType: .PortItem) else {
                continue
            }
            if deviceName.contains(r.port.name) {
                if model.isPortAssigned(portId: r.port.id) {
                    found = true
                    break
                }
            }
        }
        if !found {
            UITools.showMessage(message:
                    "Serial port must be linked with another port.")
            return
        }
        // Create open file dialog
        let openPanel = NSOpenPanel()
        openPanel.canChooseFiles = true
        openPanel.canChooseDirectories = false
        openPanel.allowsMultipleSelection = false
        //openPanel.allowedFileTypes = ["js"]
        openPanel.prompt = "Execute"

        // Create JavaScript file in App Home directory
        if let homeDir = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first {
            openPanel.directoryURL = homeDir
        }
        else if let docDir = FileManager.default.urls(for: .userDirectory, in: .userDomainMask).first {
            openPanel.directoryURL = docDir
        }
        
        // Show the dialog
        if openPanel.runModal() == .OK {
            guard let selectedFileURL = openPanel.url else {
                return
            }
            self.jsRunner.scriptFile = selectedFileURL
            self.pbIoLooper.isEnabled = false
            self.pbIoSendFile.isEnabled = false
            self.pbIoSendText.isEnabled = serialPort?.isConnected ?? false
            self.edTextField.isEnabled = serialPort?.isConnected ?? false
            self.edAutoTextLen.isEnabled = false
            self.isLooperRunning = false //teminate
        }
    }
    
    @objc func onStopScripting() {
        self.jsRunner.scriptFile = nil
        self.pbIoLooper.isEnabled = serialPort?.isConnected ?? false
        self.pbIoSendFile.isEnabled = serialPort?.isConnected ?? false
        self.pbIoSendText.isEnabled = serialPort?.isConnected ?? false
        self.edTextField.isEnabled = serialPort?.isConnected ?? false
        self.edAutoTextLen.isEnabled = serialPort?.isConnected ?? false
    }
  
    private func addEoL(_ text: String) -> String
    {
        var _text = text

        // Check if \r is second last character before \n
        if _text.count >= 2 {
            let lastTwoChars = String(_text.suffix(2))
            if lastTwoChars == "\r\n" {
                // \r is already second last character
            } else if lastTwoChars == "\n" {
                // Only \n at the end, so we need to add \r before it
                _text.insert("\r", at: _text.index(_text.endIndex, offsetBy: -1))
            } else {
                // No line ending at all
                if self.isAddCrEnabled {
                    _text += "\r"
                }
                if self.isAddLfEnabled {
                    _text += "\n"
                }
            }
        } else {
            // Text is too short
            if self.isAddCrEnabled {
                _text += "\r"
            }
            if self.isAddLfEnabled {
                _text += "\n"
            }
        }
        return _text
    }
    
    private func runScriptFile(_ isSender: Bool, _ data: Data) -> Bool
    {
        guard let scriptFile = self.jsRunner.scriptFile else {
            return false
        }
        do {
            let script = try String(
                contentsOf: scriptFile as URL,
                  encoding: .utf8)
            self.jsRunner.onStart = { message in
                self.logMessage("onStart:\n\(message)")
            }
            self.jsRunner.onComplete = { message in
                self.logMessage("onComplete:\n\(message)")
            }
            self.jsRunner.onMessage = { message in
                self.logMessage("onMessage:\n\(message)")
            }
            self.jsRunner.onSendText = { message in
                self.serialPort?.send(self.addEoL(message).data(using: .utf8)!)
            }
            self.jsRunner.setVarialble("dataAvailable", !data.isEmpty)
            self.jsRunner.setVarialble("receivedData", data)
            self.jsRunner.run(script: script)
        } catch {
            self.isLooperRunning = false // force stop if looping
            UITools.showMessage(message: "Error reading script file: \(error)")
        }
        return true
    }
    
    @objc func serialPortDidReceive(_ data: Data) {
        if !self.runScriptFile(false, data) {
            self.logMessage(toHexLog(data))
        }
    }

    @objc func serialPortStateChanged(_ state: SerialPortState) {
        DispatchQueue.main.async {
            if state == .connected {
                self.serialPortDidConnect()
            }
            if state == .disconnected {
                self.isLooperRunning = false
                self.serialPortDidDisconnect()
            }
        }
    }

    @objc func serialPortDidError(_ error: any Error, with errorType: SerialPortErrorType) {
        DispatchQueue.main.async {
            self.isLooperRunning = false
            self.logMessage("\(String(describing: error))\nType: \(errorType.rawValue)")
            self.pbPortOpen.isEnabled = true
            self.pbPortClose.isEnabled = false
            self.edTextField.isEnabled = false
            self.pbIoLooper.isEnabled = false
            //self.pbRunScript.isEnabled = false
            self.pbIoSendFile.isEnabled = false
            self.pbIoSendText.isEnabled = false
            self.edAutoTextLen.isEnabled = false
            self.cbxSendTextAddCR.isEnabled = false
            self.cbxSendTextAddLF.isEnabled = false
        }
    }

    @objc func serialPortDidConnect() {
        DispatchQueue.main.async {
            self.logMessage("Connected to serial port.")
            self.pbPortOpen.isEnabled = false
            self.pbPortClose.isEnabled = true
            self.cbxSendTextAddCR.isEnabled = true
            self.cbxSendTextAddLF.isEnabled = true
            self.edTextField.isEnabled = self.jsRunner.scriptFile == nil
            self.edAutoTextLen.isEnabled = self.jsRunner.scriptFile == nil
            self.pbIoLooper.isEnabled = self.jsRunner.scriptFile == nil
            self.pbIoSendFile.isEnabled = self.jsRunner.scriptFile == nil
            self.pbIoSendText.isEnabled = self.jsRunner.scriptFile == nil
        }
    }

    @objc func serialPortDidDisconnect() {
        DispatchQueue.main.async {
            self.isLooperRunning = false
            self.logMessage("Disconnected from serial port.")
            self.pbPortOpen.isEnabled = true
            self.pbPortClose.isEnabled = false
            self.cbxSendTextAddCR.isEnabled = false
            self.cbxSendTextAddLF.isEnabled = false
            self.edTextField.isEnabled = false
            self.pbIoLooper.isEnabled = false
            self.pbIoSendFile.isEnabled = false
            self.pbIoSendText.isEnabled = false
            self.edAutoTextLen.isEnabled = false
        }
    }

    @objc func serialPortDidDisconnect(_ error: (any Error)) {
        DispatchQueue.main.async {
            self.isLooperRunning = false
            self.logMessage("Error \(String(describing: error))")
            self.pbPortOpen.isEnabled = true
            self.pbPortClose.isEnabled = false
            self.edTextField.isEnabled = false
            self.pbIoLooper.isEnabled = false
            self.pbIoSendFile.isEnabled = false
            self.pbIoSendText.isEnabled = false
            self.edAutoTextLen.isEnabled = false
            self.cbxSendTextAddCR.isEnabled = false
            self.cbxSendTextAddLF.isEnabled = false
        }
    }
    
    @objc func serialPort(_ port: Any, didChangeBaudRate baudRate: UInt) {
        logMessage("Parameter changed. Baud rate: \(baudRate)")
    }
    
    @objc func serialPort(_ port: Any, didChangeDataBits dataBits: UInt) {
        logMessage("Parameter changed. Data bits: \(dataBits)")
    }
    
    @objc func serialPort(_ port: Any, didChangeStopBits stopBits: UInt) {
        logMessage("Parameter changed. Stop bits: \(stopBits)")
    }
    
    @objc func serialPort(_ port: Any, didChangeParity parity: UInt) {
        logMessage("Parameter changed. Parity: \(parity)")
    }
    
    @objc func serialPort(_ port: Any, didChangeFlowControl flowControl: UInt) {
        logMessage("Parameter changed. Flow control: \(flowControl)")
    }
    
    @objc func serialPort(_ port: Any, didUpdatePinoutSignals DCD: Bool, dtr DTR: Bool, dsr DSR: Bool, rts RTS: Bool, cts CTS: Bool, ri RI: Bool) {
        var notify : Bool = false        
        if (serialPins.DCD != DCD) {
            serialPins.DCD = DCD
            notify = true
        }
        if (serialPins.DTR != DTR) {
            serialPins.DTR = DTR
            notify = true
        }
        if (serialPins.DSR != DSR) {
            serialPins.DSR = DSR
            notify = true
        }
        if (serialPins.RTS != RTS) {
            serialPins.RTS = RTS
            notify = true
        }
        if (serialPins.CTS != CTS) {
            serialPins.CTS = CTS
            notify = true
        }
        if (serialPins.RI != RI) {
            serialPins.RI = RI
            notify = true
        }
        if notify {
            notifyPinsChanged(serialPins)
        }
    }

    @IBAction func onOpenPort(_ sender: NSButton) {
        txLogView.string = ""
        connectToSerialPort()
    }
    
    @IBAction func onClosePort(_ sender: NSButton) {
        serialPort?.disconnect()
        serialPort = nil
    }
    
    @IBAction func onSendFile(_ sender: NSButton) {
        let openPanel = NSOpenPanel()
        
        // Basic configuration
        openPanel.canChooseFiles = true
        openPanel.canChooseDirectories = false
        openPanel.allowsMultipleSelection = false
        
        // Show panel and handle response
        if openPanel.runModal() == .OK {
            if let fileURL = openPanel.url {
                self.logMessage(">: Send file \(fileURL.path)")
                self.jsRunner.scriptFile = nil //disable scripting
                self.pbIoSendFile.isEnabled = false
                self.serialPort?.sendFile(atPath: fileURL.path,
                                     chunkSize: 1024,
                                     completion:{_,_ in
                    DispatchQueue.main.async {
                        self.pbIoSendFile.isEnabled = true
                    }
                })
            }
        }
    }
    
    @IBAction func onSendText(_ sender: NSButton) {
        if textToSend.isEmpty {
            textToSend = edTextField.stringValue
        }
        let text = self.addEoL(textToSend)
        logMessage(">: \(text)")
        serialPort?.send(text.data(using: .utf8)!)
    }
    
    @IBAction func onRunLooper(_ sender: NSButton) {
        if !isLooperRunning && serialPort != nil {
            onTextLengthChanged(edAutoTextLen)
            DispatchQueue.global().async {
                let chars: String = "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
                var index = 0;
                self.isLooperRunning = true
                while self.isLooperRunning && self.serialPort != nil {
                    var buffer : String = ""
                    for _ in 0..<self.autoTextLen {
                        buffer.append(chars[index])
                    }
                    index += 1
                    if index >= chars.count {
                        index = 0
                    }
                    if (!buffer.contains("\r") && self.isAddCrEnabled) {
                        buffer += "\r"
                    }
                    if (!buffer.endsWith("\n") && self.isAddLfEnabled) {
                        buffer += "\n"
                    }
                    
                    self.logMessage(">: \(buffer)")
                    self.serialPort?.send(buffer.data(using: .utf8)!)
                
                    Thread.sleep(forTimeInterval: 0.5)
                }
            }
        } else {
            isLooperRunning = false
        }
    }
    
    @IBAction func onRunScript(_ sender: NSButton) {
        // Create popup menu
        let menu = NSMenu()
        
        // Create "New Script" menu item with icon
        let newScriptItem = NSMenuItem(title: "New Script", action: #selector(onNewScript), keyEquivalent: "")
        newScriptItem.image = NSImage(systemSymbolName: "plus.circle", accessibilityDescription: "New Script")
        menu.addItem(newScriptItem)
        
        // Create "Execute Script" menu item with icon
        let startScriptItem = NSMenuItem(title: "Start Scripting", action: #selector(onStartScripting), keyEquivalent: "")
        startScriptItem.image = NSImage(systemSymbolName: "play.circle", accessibilityDescription: "Start Scripting")
        menu.addItem(startScriptItem)

        let stopScriptItem = NSMenuItem(title: "Stop Scripting", action: #selector(onStopScripting), keyEquivalent: "")
        stopScriptItem.image = NSImage(systemSymbolName: "stop.circle", accessibilityDescription: "Stop Scripting")
        menu.addItem(stopScriptItem)

        // Calculate bottom-left corner of the button in its own coordinate space
        let popupPoint = NSPoint(x: 0, y: 0)
        
        // Show the popup menu at bottom-left of the button
        menu.popUp(positioning: nil, at: popupPoint, in: sender)
    }
  
    // This method is called when the user presses Enter or Return
    func controlTextDidEndEditing(_ obj: Notification) {
        if let textField = obj.object as? NSTextField {
            if textField == self.edTextField {
                textToSend = edTextField.stringValue
                onSendText(pbIoSendText)
            }
        }
    }

    @IBAction func onTextLengthChanged(_ sender: NSTextField) {
        guard let value : UInt32 = sender.stringValue.uint32Value, value > 0, value <= UInt32.max else {
            autoTextLen = 64
            return
        }
        autoTextLen = value
    }
    
    @IBAction func onSendTextAddCR(_ sender: NSButton) {
        isAddCrEnabled = (sender.state == .on)
    }
    
    @IBAction func onSendTextAddLF(_ sender: NSButton) {
        isAddLfEnabled = (sender.state == .on)
    }
    
    @IBAction func onTextChanged(_ sender: NSTextField) {
        textToSend = edTextField.stringValue
    }
    
    @IBAction func onSerialPortChanged(_ sender: ComboBox) {
        guard let value = UITools.selectedValueFrom(sender) as? String else {
            return
        }
        self.title = "Serial Test [\(value)]"
        deviceName = value
        pbPortOpen.isEnabled = true;
    }
    
    @IBAction func onBaudRateChanged(_ sender: ComboBox) {
        guard let value = UITools.selectedValueFrom(sender) as? UInt32 else {
            return
        }
        portConfig.baudRate = UInt(value)
    }
    
    @IBAction func onDataBitsChanged(_ sender: ComboBox) {
        guard let value = UITools.selectedValueFrom(sender) as? UInt8 else {
            return
        }
        portConfig.dataBits = UInt(value)
    }
    
    @IBAction func onStopBitsChanged(_ sender: ComboBox) {
        guard let value = UITools.selectedValueFrom(sender) as? UInt8 else {
            return
        }
        portConfig.stopBits = UInt(value)
    }
    
    @IBAction func onParityChanged(_ sender: ComboBox) {
        guard let value = UITools.selectedValueFrom(sender) as? UInt8 else {
            return
        }
        portConfig.parity = UInt(value)
    }
    
    @IBAction func onFlowCtrlChanged(_ sender: ComboBox) {
        guard let value = UITools.selectedValueFrom(sender) as? UInt8 else {
            return
        }
        portConfig.flowCtrl = UInt(value)
    }
}

extension SPTestViewController: NSOpenSavePanelDelegate {
    func panel(_ sender: Any, shouldEnable url: URL) -> Bool {
        // Filter out system files if needed
        return !url.path.startsWith(".")
    }
}
