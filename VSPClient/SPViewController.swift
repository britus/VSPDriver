// ********************************************************************
// VSPClient User Interface
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
import Foundation
import Cocoa
import AppKit

class SPViewController: NSViewController, NSTableViewDelegate, NSTableViewDataSource, DataModelObserver {
    @IBOutlet weak var tableView: TableView!
    @IBOutlet weak var pnlSerialPort: NSView!
    @IBOutlet weak var cbxBaudRate: ComboBox!
    @IBOutlet weak var cbxDataBits: ComboBox!
    @IBOutlet weak var cbxStopBits: ComboBox!
    @IBOutlet weak var cbxParity: ComboBox!
    @IBOutlet weak var cbxFlowCtrl: ComboBox!
    @IBOutlet weak var tbvPortList: TableView!
    @IBOutlet weak var pbCreatePort: NSButton!
    
    private var model: DataModel = DataModel.shared

    var hoveredRow: Int = -1
    lazy var hoverButton: NSButton = createHoverButton()
    
    private var m_baudRate: UInt8 = 0
    private var m_dataBits: UInt8 = 0
    private var m_stopBits: UInt8 = 0
    private var m_parity: UInt8 = 0
    private var m_flowCtrl: UInt8 = 0

    override func viewDidLoad() {
        super.viewDidLoad()
        tableView.delegate = self
        tableView.dataSource = self
        
        // Enable row tracking
        tableView.addTrackingArea(NSTrackingArea(//
            rect: tableView.bounds,
            options: [.mouseMoved, .activeAlways, .inVisibleRect],
            owner: self,
            userInfo: nil))
        
        // Attach contextual menu
        tableView.menu = createPopupMenu()

        // serial port parameters
        populateComboBoxes()
    }
    
    override func viewDidAppear() {
        super.viewDidAppear()
        model.addObserver(self)
        tableView.reloadData()
    }
    
    override func viewWillDisappear() {
        model.removeObserver(self)
        super.viewWillDisappear()
    }

    override func mouseMoved(with event: NSEvent) {
        let location = tableView.convert(event.locationInWindow, from: nil)
        let row = tableView.row(at: location)

        if row != hoveredRow {
            hoveredRow = row
            updateHoverButtonPosition()
        }
    }

    private func updateHoverButtonPosition() {
        hoverButton.isHidden = (hoveredRow < 0)

        guard hoveredRow >= 0 else { return }

        let rect = tableView.rect(ofRow: hoveredRow)
        let converted = tableView.convert(rect, to: view)

        hoverButton.frame = NSRect(
            x: converted.maxX - 36,
            y: converted.midY - 8,
            width: 24,
            height: 16
        )
    }
    
    private func populateComboBoxes() {
        UITools.populateBaudRateComboBox(cbxBaudRate)
        UITools.populateDataBitsComboBox(cbxDataBits)
        UITools.populateStopBitsComboBox(cbxStopBits)
        UITools.populateParityComboBox(cbxParity)
        UITools.populateFlowControlComboBox(cbxFlowCtrl)
    }

    @IBAction func onBaudRateChanged(_ sender: NSComboBox) {
        if sender.indexOfSelectedItem > -1 {
            m_baudRate = UInt8(sender.indexOfSelectedItem)
        }
    }
    
    @IBAction func onDataBitsChanged(_ sender: NSComboBox) {
        if sender.indexOfSelectedItem > -1 {
            m_dataBits = UInt8(sender.indexOfSelectedItem)
        }
    }
    
    @IBAction func onStopBitsChanged(_ sender: NSComboBox) {
        if sender.indexOfSelectedItem > -1 {
            m_stopBits = UInt8(sender.indexOfSelectedItem)
        }
    }
    
    @IBAction func onParityChanged(_ sender: NSComboBox) {
        if sender.indexOfSelectedItem > -1 {
            m_parity = UInt8(sender.indexOfSelectedItem)
        }
    }
    
    @IBAction func onFlowCtrlChanged(_ sender: NSComboBox) {
        if sender.indexOfSelectedItem > -1 {
            m_flowCtrl = UInt8(sender.indexOfSelectedItem)
        }
    }
    
    @IBAction func onCreatePort(_ sender: NSButton) {
        // load selection
        onBaudRateChanged(cbxBaudRate)
        onDataBitsChanged(cbxDataBits)
        onStopBitsChanged(cbxStopBits)
        onParityChanged(cbxParity)
        onFlowCtrlChanged(cbxFlowCtrl)
        // C Bridge
        DispatchQueue.global(qos: .background).asyncAfter(//
            deadline: .now() + .milliseconds(100)) {
            CreatePort(self.m_baudRate, self.m_dataBits, self.m_stopBits, self.m_parity, self.m_flowCtrl)
        }
    }
    
    func numberOfRows(in tableView: NSTableView) -> Int {
        return model.recordCount(byType: TDataType.PortItem)
    }
    
    func numberOfSections(in tableView: NSTableView) -> Int {
        return 1
    }
    
    func tableView(_ tableView: NSTableView, numberOfRowsInSection section: Int) -> Int {
        return model.recordCount(byType: TDataType.PortItem)
    }
    
    func tableView(_ tableView: NSTableView, viewFor tableColumn: NSTableColumn?, row: Int) -> NSView? {
        
        guard let record = model.getRecord(index: row, byType: TDataType.PortItem),
              let tableColumn = tableColumn else {
            UITools.showMessage(message: "Invalid table column or record index")
            return nil
        }
        
        let colId = tableColumn.identifier
        
        // Reuse an NSTableCellView from Interface Builder or programmatically
        if let cellView = tableView.makeView(withIdentifier: colId, owner: self) as? NSTableCellView {
            if record.type == .PortItem {
                switch colId.rawValue {
                case "tcPortId":
                    cellView.textField?.stringValue = "\(record.port.id)"
                case "tcPortName":
                    cellView.textField?.stringValue = record.port.name
                default:
                    cellView.textField?.stringValue = "-"
                }
            }
            return cellView
        }
        
        return nil
    }
    
    func dataChanged(_ data: [TDataRecord]?) {
        guard data == data else {
            return
        }
        DispatchQueue.main.async {
            self.tableView.beginUpdates()
            self.tableView.reloadData()
            self.tableView.endUpdates()
            self.tableView.isEnabled = !(data?.isEmpty ?? false)
            if (self.tableView.isEnabled) {
                self.tableView.selectRowIndexes(IndexSet(//
                    integer: self.hoveredRow > 0 ? self.hoveredRow - 1 : 0),//
                                                byExtendingSelection: false)
            }
            self.pbCreatePort.isEnabled = self.tableView.numberOfRows < MAX_SERIAL_PORTS
        }
    }
    
    private func createPopupMenu() -> NSMenu {
        let menu = NSMenu(title: "Select")
        //menu.addItem(withTitle: "Link", action: #selector(onLinkTo(_:)), keyEquivalent: "")
        menu.addItem(withTitle: "Delete", action: #selector(onDelete(_:)), keyEquivalent: "")
        return menu
    }
#if false
    @objc func onLinkTo(_ sender: Any?) {
        guard hoveredRow >= 0, let record = //
                model.getRecord(index: hoveredRow, byType: TDataType.PortItem) else {
            return
        }
        // TODO: Link creation dialog
        print("onLinkTo: \(record)")
    }
#endif // false
    @objc func onDelete(_ sender: Any?) {
        guard hoveredRow >= 0, let record : TDataRecord = //
                model.getRecord(index: hoveredRow, byType: TDataType.PortItem) else {
            return
        }
        if record.type != .PortItem {
            UITools.showMessage(message: "Can't delete non-port item")
            return
        }
        hoverButton.isHidden = true
        if UITools.showQuestionDialog(self, "Delete this record?") {
            DispatchQueue.global(qos: .background).asyncAfter(//
                deadline: .now() + .milliseconds(100)) {
                RemovePort(record.port.id)
                GetStatus()
            }
            model.removeRecord(index: hoveredRow)
        }
    }
    
    private func createHoverButton() -> NSButton {
        let btn = NSButton(title: "⋯", target: self, action: #selector(hoverButtonClicked(_:)))
        btn.bezelStyle = .inline
        btn.isBordered = true
        btn.isHidden = true  // hidden until hover
        view.addSubview(btn)
        return btn
    }

    @objc func hoverButtonClicked(_ sender: NSButton) {
        NSMenu.popUpContextMenu(createPopupMenu(), with: NSApp.currentEvent!, for: sender)
        hoverButton.isHidden = true
    }
}
