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

    var selectedRow: Int = -1
    lazy var hoverButton: NSButton = createHoverButton()
    private var parameters: SerialPortParameters = SerialPortParameters()

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

    // Handle key events for the table view
    override func keyDown(with event: NSEvent) {
        // Backspace key code
        if event.keyCode == 51 {
            selectedRow = tableView.selectedRow
            onDelete(tableView)
            return
        }

        super.keyDown(with: event)
    }

    override func mouseMoved(with event: NSEvent) {
        let location = tableView.convert(event.locationInWindow, from: nil)
        let row = tableView.row(at: location)

        if row != selectedRow {
            selectedRow = row
            updateHoverButtonPosition()
        }
    }

    private func updateHoverButtonPosition() {
        hoverButton.isHidden = (selectedRow < 0)

        guard selectedRow >= 0 else { return }

        let rect = tableView.rect(ofRow: selectedRow)
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

    @IBAction func onBaudRateChanged(_ sender: ComboBox) {
        guard let value = UITools.selectedValueFrom(sender) as? UInt32 else {
            return
        }
        parameters.baudRate = UInt(value)
    }
    
    @IBAction func onDataBitsChanged(_ sender: ComboBox) {
        guard let value = UITools.selectedValueFrom(sender) as? UInt8 else {
            return
        }
        parameters.dataBits = UInt(value)
    }
    
    @IBAction func onStopBitsChanged(_ sender: ComboBox) {
        guard let value = UITools.selectedValueFrom(sender) as? UInt8 else {
            return
        }
        parameters.stopBits = UInt(value)
    }
    
    @IBAction func onParityChanged(_ sender: ComboBox) {
        guard let value = UITools.selectedValueFrom(sender) as? UInt8 else {
            return
        }
        parameters.parity = UInt(value)
    }
    
    @IBAction func onFlowCtrlChanged(_ sender: ComboBox) {
        guard let value = UITools.selectedValueFrom(sender) as? UInt8 else {
            return
        }
        parameters.flowCtrl = UInt(value)
    }
    
    @IBAction func onCreatePort(_ sender: NSButton) {
        // load selection
        onBaudRateChanged(cbxBaudRate)
        onDataBitsChanged(cbxDataBits)
        onStopBitsChanged(cbxStopBits)
        onParityChanged(cbxParity)
        onFlowCtrlChanged(cbxFlowCtrl)
        showProgress()
        // C Bridge
        DispatchQueue.global(qos: .background).asyncAfter(//
             deadline: .now() + .milliseconds(100)) {
            CreatePort(UInt32(self.parameters.baudRate),
                       UInt8(self.parameters.dataBits),
                       UInt8(self.parameters.stopBits),
                       UInt8(self.parameters.parity),
                       UInt8(self.parameters.flowCtrl))
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
                    integer: self.selectedRow > 0 ? self.selectedRow - 1 : 0),//
                                                byExtendingSelection: false)
            }
            self.pbCreatePort.isEnabled = self.tableView.numberOfRows < MAX_SERIAL_PORTS
            self.hideProgress()
        }
    }
    
    private func createPopupMenu() -> NSMenu {
        let menu = NSMenu(title: "Select")
        //menu.addItem(withTitle: "Link", action: #selector(onLinkTo(_:)), keyEquivalent: "")
        menu.addItem(withTitle: "Delete", action: #selector(onDelete(_:)), keyEquivalent: "")
        return menu
    }
    
    @objc func onDelete(_ sender: Any?) {
        guard selectedRow >= 0, let record : TDataRecord = //
                model.getRecord( index: selectedRow,
                                byType: TDataType.PortItem)
        else {
            return
        }
        if record.type != .PortItem {
            UITools.showMessage(message: "Can't delete non-port item")
            return
        }
        hoverButton.isHidden = true
        if UITools.showQuestionDialog(self, "Delete this record?") {
            showProgress()
            DispatchQueue.global(qos: .background).asyncAfter(//
                deadline: .now() + .milliseconds(100)) {
                RemovePort(record.port.id)
                GetStatus()
            }
            model.removeRecord(index: selectedRow)
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
