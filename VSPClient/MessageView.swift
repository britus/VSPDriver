// ********************************************************************
// VSPClient User Interface
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
import Cocoa

class MessageView: NSViewController, DriverDataObserver {
    @IBOutlet weak var cbxCheckBaudRate: NSButton!
    @IBOutlet weak var cbxCheckFlowCtrl: NSButton!
    @IBOutlet weak var cbxCheckParity: NSButton!
    @IBOutlet weak var cbxCheckStopBits: NSButton!
    @IBOutlet weak var cbxCheckDataBits: NSButton!
    @IBOutlet weak var cbxTraceRx: NSButton!
    @IBOutlet weak var cbxTraceCtrl: NSButton!
    @IBOutlet weak var cbxTraceTx: NSButton!
    @IBOutlet var txLogView: NSTextView!
    @IBOutlet weak var pbSaveTraces: NSButton!
    
    private let manager: DriverManager = DriverManager.shared
    private var msgProxy : MessageProxy? = nil;
    private var m_checkFlags : UInt64 = 0
    private var m_traceFlags : UInt64 = 0
    
    override func viewDidLoad() {
        super.viewDidLoad()
        msgProxy = MessageProxy(parent: self, manager: manager)
        txLogView.setLineWrapping(false)
    }
    
    override func viewWillAppear() {
        super.viewDidAppear()
        manager.addObserver(self)
    }
    
    override func viewWillDisappear() {
        manager.removeObserver(self)
        super.viewWillDisappear()
    }
    
    @IBAction func onSaveTraces(_ sender: NSButton) {
        self.showProgress()
        DispatchQueue.global(qos: .background).asyncAfter(//
            deadline: .now() + .milliseconds(50)) {
                EnableChecksAndTrace(0, self.m_checkFlags, self.m_traceFlags)
                GetStatus()
        }
   }
    
    let TRACE_PORT_RX : UInt8 = 16
    let TRACE_PORT_TX : UInt8 = 17
    let TRACE_PORT_IO : UInt8 = 18
    
    let CHECK_BAUD : UInt8 = 19
    let CHECK_DATA_SIZE : UInt8 = 20
    let CHECK_STOP_BITS : UInt8 = 21
    let CHECK_PARITY : UInt8 = 22
    let CHECK_FLOWCTRL : UInt8 = 23

    @IBAction func onBaudRateChanged(_ sender: NSButton) {
        m_checkFlags = bitUpdate(CHECK_BAUD, m_checkFlags, isSet: sender.state == .on)
    }
    
    @IBAction func onDataBitsChanged(_ sender: NSButton) {
        m_checkFlags = bitUpdate(CHECK_DATA_SIZE, m_checkFlags, isSet: sender.state == .on)
    }
    
    @IBAction func onStopBitsChanged(_ sender: NSButton) {
        m_checkFlags = bitUpdate(CHECK_STOP_BITS, m_checkFlags, isSet: sender.state == .on)
    }
    
    @IBAction func onParityChanged(_ sender: NSButton) {
        m_checkFlags = bitUpdate(CHECK_PARITY, m_checkFlags, isSet: sender.state == .on)
    }
    
    @IBAction func onFlowCtrlChanged(_ sender: NSButton) {
        m_checkFlags = bitUpdate(CHECK_FLOWCTRL, m_checkFlags, isSet: sender.state == .on)
    }

    @IBAction func onTraceRxChanged(_ sender: NSButton) {
        m_traceFlags = bitUpdate(TRACE_PORT_RX, m_traceFlags, isSet: sender.state == .on)
    }
    
    @IBAction func onTraceTxChanged(_ sender: NSButton) {
        m_traceFlags = bitUpdate(TRACE_PORT_TX, m_traceFlags, isSet: sender.state == .on)
    }
    
    @IBAction func onTraceCtrlChanged(_ sender: NSButton) {
        m_traceFlags = bitUpdate(TRACE_PORT_IO, m_traceFlags, isSet: sender.state == .on)
    }

    private func bitUpdate(_ bit: UInt8, _ mask: UInt64, isSet: Bool) -> UInt64 {
        if isSet {
            return mask | toBit(bit)
        } else {
            return mask & ~toBit(bit)
        }
    }
    
    private func toBit(_ bit: UInt8) -> UInt64 {
        return UInt64(1 << bit)
    }
    
    private func isBit(_ id: UInt8, _ mask : UInt64) -> Bool {
        return ((toBit(id) & mask) == toBit(id))
    }
    
    open func logMessage(_ msg: String)
    {
        DispatchQueue.main.async {
            var _text = msg.trimmingCharacters(in: CharacterSet(charactersIn: "\r\n"))
            if !_text.endsWith("\n") {
                _text.append("\n")
            }
            let _buffer = (self.txLogView.string as NSString).appending(_text)
            self.txLogView.setText(_buffer)
            //self.txLogView.scrollToTextEnd()
            self.txLogView.scrollToTextViewEnd()
        }
    }

    func dataDidAvailable(_ data: TVSPControllerData?) {
        guard let d : TVSPControllerData = data else {
            return
        }
        DispatchQueue.main.async { [self] in
            self.m_traceFlags = d.traceFlags
            self.m_checkFlags = d.checkFlags
            self.cbxCheckBaudRate.state = isBit(CHECK_BAUD, d.checkFlags) ? .on : .off
            self.cbxCheckDataBits.state = isBit(CHECK_DATA_SIZE, d.checkFlags) ? .on : .off
            self.cbxCheckStopBits.state = isBit(CHECK_STOP_BITS, d.checkFlags) ? .on : .off
            self.cbxCheckParity.state = isBit(CHECK_PARITY, d.checkFlags) ? .on : .off
            self.cbxCheckFlowCtrl.state = isBit(CHECK_FLOWCTRL, d.checkFlags) ? .on : .off
            self.cbxTraceRx.state = isBit(TRACE_PORT_RX, d.traceFlags) ? .on : .off
            self.cbxTraceTx.state = isBit(TRACE_PORT_TX, d.traceFlags) ? .on : .off
            self.cbxTraceCtrl.state = isBit(TRACE_PORT_IO, d.traceFlags) ? .on : .off
            self.hideProgress()
        }
    }
}

class MessageProxy: DriverManagerObserver {
    let parent: MessageView
    let manager: DriverManager
    
    init (parent _parent: MessageView, manager _manager: DriverManager) {
        parent = _parent
        manager = _manager
        manager.addObserver(self)
    }
    
    deinit {
        manager.removeObserver(self)
    }
    
    func logMessageDidAvailable(_ message: String?) {
        guard let msg : String = message else {
            return;
        }
        parent.logMessage(msg)
    }
    
    func driverStatusDidChange(_ status: DriverStatus, code: UInt64, domain: String, message: String) {
        parent.logMessage("Driver status: \(status.rawValue), code: \(code), message:\n\(message)")
    }
}
