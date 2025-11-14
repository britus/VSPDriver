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
    private var imgFirstTinted: NSImage?
    private var imgSecondTinted: NSImage?
    private var imgOriginalIoLoop: NSImage?
    private var imgOriginalScript: NSImage?
    private var scriptFile: URL?
    private var fileHistory: FileHistory = FileHistory.shared
    
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
        
        let formatter = NumberFormatter()
        formatter.numberStyle = .none               // or .currency, .percent, etc.
        formatter.usesGroupingSeparator = false     // ← disables thousands separators
        formatter.maximumFractionDigits = 0         // optional, for precision control
        formatter.minimumFractionDigits = 0
        edAutoTextLen.formatter = formatter
      
        if let originalImage = pbIoLooper.image {
            imgOriginalIoLoop = originalImage
            // Create first tinted version
            imgFirstTinted = tintImage(originalImage, with: NSColor.red)
        }
        if let originalImage = pbRunScript.image {
            imgOriginalScript = originalImage
            // Create second tinted version
            imgSecondTinted = tintImage(originalImage, with: NSColor.green)
        }

        // catch view/window close event
        onWindowClose { [weak self] in
            guard let self = self else {
                return
            }
            viewWillClose()
        }

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
    
    override func viewWillClose() {
        self.isLooperRunning = false
        DispatchQueue.global().async {
            self.serialPort?.disconnect()
            self.serialPort = nil
        }
    }
    
    deinit {
        removeWindowCloseHandler()
    }
    
    private func tintImage(_ image: NSImage, with color: NSColor) -> NSImage {
        guard let _ = image.copy() as? NSImage else {
            return image
        }
        
        // Create a new image with the same size
        let newImage = NSImage(size: image.size)
        newImage.lockFocus()
        
        // Fill with the tint color
        color.set()
        NSRect(origin: .zero, size: image.size).fill()
        
        // Draw the original image on top with sourceAtop blending
        image.draw(in: NSRect(origin: .zero, size: image.size),
                   from: NSRect(origin: .zero, size: image.size),
                   operation: .destinationIn,
                   fraction: 1.0)
        
        newImage.unlockFocus()
        return newImage
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
        //logMessage("scriptExecutionDidStart: \(context)")
    }
    
    func scriptExecutionDidFinish(_ context: JSContext) {
        //logMessage("scriptExecutionDidFinish: \(context)")
    }
    
    func scriptExecutionDidFail(_ message: String) {
        self.logMessage("SP: Error -> \(message)")
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
        
            // Using the value of the global property
            // named "receivedData". The value type 
            // is a byte array. [UInt8]
            if (dataAvailable === true) {
                switch (parseInt(receivedData[0])) {
                    case 50: {
                        onSendText("ATQ\\r\\nOK\\r\\n");
                        break;
                    }        
                    case 51: {
                        onSendText("AT\\r\\nOK\\r\\n");
                        break;
                    }
                    case 52: {
                        onSendText("ATZ\\r\\nOK\\r\\n");
                        break;
                    }
                    case 53: {
                        onSendText(":CPIN=?\\r\\nOK\\r\\n");
                        break;
                    }
                    case 54: {
                        onSendText(":ATI\\r\\nOK\\r\\n");
                        break;
                    }
                    case 55: {
                        onSendText("AT0\\r\\nOK\\r\\n");
                        break;
                    }
                    case 57: {
                        onSendText("AT1\\r\\nOK\\r\\n");
                        break;
                    }
                    default: {
                        onMessage("JS: Data=" + receivedData);
                        break;
                    }    
                }
            }
            
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
            if let _url = fileHistory.addToHistory(url) {
                NSWorkspace.shared.open(_url)
            }
        } catch {
            UITools.showMessage(message: "Error creating script file: \(error)")
        }
    }
    
    private func isPortLinked() -> Bool {
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
            return false
        }

        return true
    }
    
    @objc func onStartScriptingEx(_ fileUrl: URL?) {
        if let url = fileUrl {
            // check valid VSP port
            if !isPortLinked() {
                return
            }
            self.scriptFile = url
            self.pbIoLooper.isEnabled = false
            self.pbIoSendFile.isEnabled = false
            self.pbIoSendText.isEnabled = serialPort?.isConnected ?? false
            self.edTextField.isEnabled = serialPort?.isConnected ?? false
            self.edAutoTextLen.isEnabled = false
            self.isLooperRunning = false //teminate
            if let img = self.imgSecondTinted {
                self.pbRunScript.image = img
            }
        }
    }

    @objc func onOpenFromHistory(sender: NSMenuItem) {
        if !UITools.showQuestionDialog(self, "Open selected file in editor?") {
            onStartScriptingEx(fileHistory.url(at: sender.tag))
        } else {
            fileHistory.openFile(at: sender.tag)
        }
    }
    
    @objc func onDeleteHistory(sender: NSMenuItem) {
        if UITools.showQuestionDialog(self, "Do you want to delete file history?") {
            fileHistory.clearHistory()
        }
    }
    
    @objc func onOpenSampleFile(sender: NSMenuItem) {
        if let url = Bundle.main.url(forResource: "quectel-sim", withExtension: "js") {
            NSWorkspace.shared.open(url)
        }
    }
    
    @objc func onStartScripting(sender: NSMenuItem) {
        onStartScriptingEx(fileHistory.openFileAndAddToHistory())
    }
    
    @objc func onStopScripting(sender: NSMenuItem) {
        self.scriptFile = nil
        self.pbIoLooper.isEnabled = serialPort?.isConnected ?? false
        self.pbIoSendFile.isEnabled = serialPort?.isConnected ?? false
        self.pbIoSendText.isEnabled = serialPort?.isConnected ?? false
        self.edTextField.isEnabled = serialPort?.isConnected ?? false
        self.edAutoTextLen.isEnabled = serialPort?.isConnected ?? false
        if let img = self.imgOriginalScript {
            self.pbRunScript.image = img
        }
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
                if self.isAddCrEnabled {
                    _text.insert("\r", at: _text.index(_text.endIndex, offsetBy: -1))
                }
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
    
    private var callBalance : Int = 0
    
    private func runScriptFile(_ isSender: Bool, _ data: Data) -> Bool
    {
        guard let url : URL = self.scriptFile else {
            NSLog("SP: No script file url available, skip.")
            return false
        }
        guard url.startAccessingSecurityScopedResource() else {
            logMessage("SP: Unable to access script file: \(url.relativePath)")
            return false
        }
        guard callBalance == 0 else {
            NSLog("SP: Script is stuck. balance=\(callBalance)")
            return false
        }
       
        // force stop if looping
        self.isLooperRunning = false
        
        // running...
        callBalance += 1;
        
        do {
            let script = try String(contentsOf: url, encoding: .utf8)
            url.stopAccessingSecurityScopedResource()
            
            let jsRunner: JSRunner = JSRunner()
           
            jsRunner.delegate = self
            
            jsRunner.onStart = { message in
                self.logMessage("onStart: \(message)")
            }
            
            jsRunner.onComplete = { message in
                self.logMessage("onComplete: \(message)")
            }
            
            jsRunner.onMessage = { message in
                self.logMessage("onMessage: \(message)")
            }
            
            jsRunner.onSendText = { message in
                self.serialPort?.send(self.addEoL(message).data(using: .utf8)!)
            }
            
            jsRunner.setVarialble("dataAvailable", data.isEmpty == false)
            jsRunner.setVarialble("receivedData", data)
            jsRunner.run(script: script)
                  
        } catch {
            // stop scripting first!
            url.stopAccessingSecurityScopedResource()
            scriptFile = nil
            DispatchQueue.main.async {
                // for UI update
                self.onStopScripting(sender: NSMenuItem())
                // notify
                UITools.showAlert(
                             title: UITools.applicationName(),
                           message: "Error reading script file:\n\(error.localizedDescription)",
                        completion: {
                })
            }
        }
         
        //NSLog("SP: (runScriptFile) exit")
        callBalance -= 1;
        return true
    }
    
    // in serial port 'reader' queue
    @objc func serialPortDidReceive(_ data: Data) {
        //NSLog("serialPortDidReceive ...")
        DispatchQueue.global().sync {
            if !self.runScriptFile(false, data) {
                self.logMessage(self.toHexLog(data))
            }
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
            self.edTextField.isEnabled = self.scriptFile == nil
            self.edAutoTextLen.isEnabled = self.scriptFile == nil
            self.pbIoLooper.isEnabled = self.scriptFile == nil
            self.pbIoSendFile.isEnabled = self.scriptFile == nil
            self.pbIoSendText.isEnabled = self.scriptFile == nil
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
        isLooperRunning = false
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
                self.scriptFile = nil //disable scripting
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
            if let img = self.imgFirstTinted {
                self.pbIoLooper.image = img
            }
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
                DispatchQueue.main.async {
                    if let img = self.imgOriginalIoLoop {
                        self.pbIoLooper.image = img
                    }
                }
            }
        } else {
            isLooperRunning = false
            if let img = self.imgOriginalIoLoop {
                self.pbIoLooper.image = img
            }
        }
    }
    
    @IBAction func onRunScript(_ sender: NSButton) {
        // Create popup menu
        let menu = NSMenu()
        
        // Create "New Script" menu item with icon
        let newScriptItem = NSMenuItem(
            title: "New Script",
            action: #selector(onNewScript),
            keyEquivalent: "")
        newScriptItem.image = NSImage(
                    systemSymbolName: "plus.circle",
            accessibilityDescription: newScriptItem.title)
        menu.addItem(newScriptItem)
        
        // Create "Execute Script" menu item with icon
        let startScriptItem = NSMenuItem(
            title: "Start Scripting",
            action: #selector(onStartScripting),
            keyEquivalent: "")
        startScriptItem.image = NSImage(
                        systemSymbolName: "play.circle",
            accessibilityDescription: startScriptItem.title)
        menu.addItem(startScriptItem)

        let stopScriptItem = NSMenuItem(
                title: "Stop Scripting",
                action: #selector(onStopScripting),
                keyEquivalent: "")
        stopScriptItem.image = NSImage(
                        systemSymbolName: "stop.circle",
                accessibilityDescription: stopScriptItem.title)
        menu.addItem(stopScriptItem)

        menu.addItem(NSMenuItem(title: "", action: nil, keyEquivalent: ""))
                     
        // Get the URL to the bundled sample file
        if let url = Bundle.main.url(forResource: "quectel-sim", withExtension: "js") {
            let fileScriptItem = NSMenuItem(
                title: url.lastPathComponent,
                action: #selector(onOpenSampleFile),
                keyEquivalent: "")
            fileScriptItem.image = NSImage(
                        systemSymbolName: "document",
                accessibilityDescription: fileScriptItem.title)
            fileScriptItem.toolTip = url.absoluteString
            menu.addItem(fileScriptItem)
        }

        if !fileHistory.isEmpty {
            menu.addItem(NSMenuItem(title: "", action: nil, keyEquivalent: ""))
            let urls = fileHistory.allHistoryURLs()
            for (url) in urls {
                if let index = fileHistory.indexOfUrl(url) {
                    let fileScriptItem = NSMenuItem(
                        title: url.lastPathComponent,
                        action: #selector(onOpenFromHistory),
                        keyEquivalent: "")
                    fileScriptItem.tag = index
                    fileScriptItem.image = NSImage(
                                systemSymbolName: "document",
                        accessibilityDescription: fileScriptItem.title)
                    fileScriptItem.toolTip = url.absoluteString
                    menu.addItem(fileScriptItem)
                }
            }
            menu.addItem(NSMenuItem(title: "", action: nil, keyEquivalent: ""))
            let delScriptItem = NSMenuItem(
                    title: "Delete History",
                    action: #selector(onDeleteHistory),
                    keyEquivalent: "")
            delScriptItem.image = NSImage(
                            systemSymbolName: "trash",
                    accessibilityDescription: delScriptItem.title)
            menu.addItem(delScriptItem)
        }
        
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
