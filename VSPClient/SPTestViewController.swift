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

class SPTestViewController: NSViewController, SerialPortDelegate, NSTextFieldDelegate {
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
    
    override func viewDidLoad() {
        super.viewDidLoad()
        pbIoLooper.isEnabled = false
        pbIoSendFile.isEnabled = false
        pbIoSendText.isEnabled = false
        pbPortClose.isEnabled = false
        edTextField.isEnabled = false
        edTextField.delegate = self
        txLogView.string = ""
        txLogView.setLineWrapping(false)
        
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
        }
    }
    
    private func notifyPinsChanged(_ pins: SerialPortPinouts)
    {
        let text = "DTR:\(pins.DTR) DSR:\(pins.DSR) RTS:\(pins.RTS) CTS:\(pins.CTS) RI:\(pins.RI)"
        logMessage("SP-PINS: \(text)")
    }

    private func toHexLog(_ data: Data) -> String {
        if let text = String(data: data, encoding: .utf8) {
            return "<: \(text)"
        }
        
        let bytes = Array(data)
        
        // Format hex bytes (30 32 33 34 35 36 37 38 39 71 07 0D)
        let hexPart = bytes.map { String(format: "%02X", $0) }.joined(separator: " ")
        
        // Format ASCII characters (0123456789G.)
        let asciiPart = bytes.map { byte in
            if byte >= 32 && byte <= 126 {
                return String(UnicodeScalar(byte))
            } else {
                return "."
            }
        }.joined()
        
        // Format with proper spacing
        return String(format: "RECIEVED:\n%-30s | %s", hexPart, asciiPart)
    }

    private func logMessage(_ msg: String)
    {
        DispatchQueue.main.async {
            var _text = msg.trimmingCharacters(in: CharacterSet(charactersIn: "\r\n"))
            if !_text.endsWith("\n") {
                _text.append("\n")
            }
            let _buffer = (self.txLogView.string as NSString).appending(_text)
            self.txLogView.setText(_buffer)
            self.txLogView.scrollToTextEnd()
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

    @objc func serialPortDidError(_ error: any Error, with errorType: SerialPortErrorType) {
        DispatchQueue.main.async {
            self.logMessage("Error \(String(describing: error))")
            self.pbPortOpen.isEnabled = true
            self.pbPortClose.isEnabled = false
            self.edTextField.isEnabled = true
            self.pbIoLooper.isEnabled = true
            self.pbIoSendFile.isEnabled = true
            self.pbIoSendText.isEnabled = true
        }
    }

    @objc func serialPortDidConnect() {
        DispatchQueue.main.async {
            self.logMessage("Connected to serial port.")
            self.pbPortOpen.isEnabled = false
            self.pbPortClose.isEnabled = true
            self.edTextField.isEnabled = true
            self.pbIoLooper.isEnabled = true
            self.pbIoSendFile.isEnabled = true
            self.pbIoSendText.isEnabled = true
        }
    }

    @objc func serialPortDidDisconnect() {
        print("Serial port disconnected")
        DispatchQueue.main.async {
            self.logMessage("Disconnected from serial port.")
            self.pbPortOpen.isEnabled = true
            self.pbPortClose.isEnabled = false
            self.edTextField.isEnabled = false
            self.pbIoLooper.isEnabled = false
            self.pbIoSendFile.isEnabled = false
            self.pbIoSendText.isEnabled = false
        }
    }

    @objc func serialPortDidDisconnect(_ error: (any Error)) {
        DispatchQueue.main.async {
            self.logMessage("Error \(String(describing: error))")
            self.pbPortOpen.isEnabled = true
            self.pbPortClose.isEnabled = false
            self.edTextField.isEnabled = false
            self.pbIoLooper.isEnabled = false
            self.pbIoSendFile.isEnabled = false
            self.pbIoSendText.isEnabled = false
        }
    }
    
    @objc func serialPortStateChanged(_ state: SerialPortState) {
        DispatchQueue.main.async {
            if state == .connected {
                self.serialPortDidConnect()
            }
            if state == .disconnected {
                self.serialPortDidDisconnect()
            }
        }
    }

    @objc func serialPortDidReceive(_ data: Data) {
        logMessage(toHexLog(data))
    }
    
    @objc func serialPort(_ port: Any, didChangeBaudRate baudRate: UInt) {
        logMessage("SP parameter changed. Baud rate: \(baudRate)")
    }
    
    @objc func serialPort(_ port: Any, didChangeDataBits dataBits: UInt) {
        logMessage("SP parameter changed. Data bits: \(dataBits)")
    }
    
    @objc func serialPort(_ port: Any, didChangeStopBits stopBits: UInt) {
        logMessage("SP parameter changed. Stop bits: \(stopBits)")
    }
    
    @objc func serialPort(_ port: Any, didChangeParity parity: UInt) {
        logMessage("SP parameter changed. Parity: \(parity)")
    }
    
    @objc func serialPort(_ port: Any, didChangeFlowControl flowControl: UInt) {
        logMessage("SP parameter changed. Flow control: \(flowControl)")
    }
    
    @objc func serialPort(_ port: Any, didUpdatePinoutSignals DCD: Bool, dtr DTR: Bool, dsr DSR: Bool, rts RTS: Bool, cts CTS: Bool, ri RI: Bool) {
        if (serialPins.DCD != DCD) {
            serialPins.DCD = DCD
            notifyPinsChanged(serialPins)
        }
        if (serialPins.DTR != DTR) {
            serialPins.DTR = DTR
            notifyPinsChanged(serialPins)
        }
        if (serialPins.DSR != DSR) {
            serialPins.DSR = DSR
            notifyPinsChanged(serialPins)
        }
        if (serialPins.RTS != RTS) {
            serialPins.RTS = RTS
            notifyPinsChanged(serialPins)
        }
        if (serialPins.CTS != CTS) {
            serialPins.CTS = CTS
            notifyPinsChanged(serialPins)
        }
        if (serialPins.RI != RI) {
            serialPins.RI = RI
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
                #if false
                let fileManager = FileManager.default
                do {
                    let attributes = try fileManager.attributesOfItem(atPath: fileURL.path)
                    if let fileSize = attributes[.size] as? NSNumber {
                        // 1 KB = 1024 bytes
                        let maxKb = UInt64(16 * 1024)
                        if fileSize.uint64Value >= maxKb {
                            let msg = 
                              "Files larger than or equal to 16 KB cannot "
                            + "be transferred. MacOS limits this in the DriverKit sandbox."
                            UITools.showMessage(message: msg)
                            return
                        }
                    }
                } catch {
                    logMessage("Error getting file size: \(error)")
                    return
                }
                #endif
                logMessage(">: Send file \(fileURL.path)")
                pbIoSendFile.isEnabled = false
                serialPort?.sendFile(atPath: fileURL.path, chunkSize: 512, completion:{_,_ in
                    DispatchQueue.main.async {
                        self.pbIoSendFile.isEnabled = true
                        self.serialPort?.disconnect();
                        self.connectToSerialPort();
                    }
                })
            }
        }
    }
    
    @IBAction func onSendText(_ sender: NSButton) {
        if textToSend.isEmpty {
            textToSend = edTextField.stringValue
        }
        let text = textToSend + "\r\n"
        logMessage(">: \(text)")
        serialPort?.send(text.data(using: .utf8)!)
    }
    
    @IBAction func onRunLooper(_ sender: NSButton) {
        if !isLooperRunning && serialPort != nil {
            pbIoLooper.title = "LOOPING!"
            var length = edTextField.stringValue.count
            if length == 0 {
                length = 16
            }
            DispatchQueue.global().async {
                let chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
                self.isLooperRunning = true
                while self.isLooperRunning {
                    var buffer : String = ""
                    for _ in 0..<length {
                        let randomIndex = Int.random(in: 0..<chars.count)
                        let char = chars[chars.index(chars.startIndex, offsetBy: randomIndex)]
                        buffer.append(char)
                    }
                    Thread.sleep(forTimeInterval: 1.500) // 1.5 seconds
                    self.serialPort?.send(buffer.data(using: .utf8)!)
                    self.logMessage(">: \(buffer)")
                }
            }
        } else {
            isLooperRunning = false
            pbIoLooper.title = "Loop"
        }
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

    @IBAction func onTextChanged(_ sender: NSTextField) {
        textToSend = edTextField.stringValue
    }
    
    @IBAction func onSerialPortChanged(_ sender: ComboBox) {
        guard let value = UITools.selectedValueFrom(sender) as? String else {
            return
        }
        deviceName = value
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
