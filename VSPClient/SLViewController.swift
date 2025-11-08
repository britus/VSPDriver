// ********************************************************************
// VSPClient User Interface
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
import Cocoa

class SLViewController: NSViewController, NSTableViewDelegate, NSTableViewDataSource, DataModelObserver {
    
    @IBOutlet weak var tableView: TableView!
    @IBOutlet weak var cbxFirstPort: ComboBox!
    @IBOutlet weak var cbxSecondPort: ComboBox!
    @IBOutlet weak var pbCreateLink: NSButton!
    
    private var model: DataModel = DataModel.shared
    private var m_firstPort : UInt8 = 0
    private var m_secondPort: UInt8 = 0
    
    var selectedRow: Int = -1
    lazy var hoverButton: NSButton = createHoverButton()
    
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
    }
    
    override func viewDidAppear() {
        super.viewDidAppear()
        self.m_firstPort = 0;
        self.m_secondPort = 0;
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
    
    func updateHoverButtonPosition() {
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
     
    @IBAction func onFirstPortSelected(_ sender: ComboBox) {
        let index = sender.indexOfSelectedItem
        guard let port = sender.dataObject(forRow: index) as? TPortItem else {
            UITools.showMessage(message: "Invalid object of selected index \(sender.indexOfSelectedItem)")
            return
        }
        m_firstPort = port.id
        self.pbCreateLink.isEnabled = (self.m_firstPort != self.m_secondPort)
    }
    
    @IBAction func onSecondPortSelected(_ sender: ComboBox) {
        let index = sender.indexOfSelectedItem
        guard let port = sender.dataObject(forRow: index) as? TPortItem else {
            UITools.showMessage(message: "Invalid object of selected index \(sender.indexOfSelectedItem)")
            return
        }
        m_secondPort = port.id
        self.pbCreateLink.isEnabled = (self.m_firstPort != self.m_secondPort)
    }
    
    @IBAction func onCreateLink(_ sender: NSButton) {
        self.showProgress()
        DispatchQueue.global(qos: .background).asyncAfter(//
            deadline: .now() + .milliseconds(100)) {
            LinkPorts(self.m_firstPort, self.m_secondPort)
            GetStatus()
        }
    }
    
    func numberOfRows(in tableView: NSTableView) -> Int {
        return model.recordCount(byType: TDataType.PortLink)
    }
    
    func numberOfSections(in tableView: NSTableView) -> Int {
        return 1
    }
    
    func tableView(_ tableView: NSTableView, numberOfRowsInSection section: Int) -> Int {
        return model.recordCount(byType: TDataType.PortLink)
    }
    
    func tableView(_ tableView: NSTableView, viewFor tableColumn: NSTableColumn?, row: Int) -> NSView? {
        
        guard let record = model.getRecord(index: row, byType: TDataType.PortLink),
              let tableColumn = tableColumn else {
            UITools.showMessage(message: "Invalid table column or record index")
            return nil
        }
        
        let colId = tableColumn.identifier
        
        // Reuse an NSTableCellView from Interface Builder or programmatically
        if let cellView = tableView.makeView(withIdentifier: colId, owner: self) as? NSTableCellView {
            if record.type == .PortLink {
                switch colId.rawValue {
                case "tcLinkId":
                    cellView.textField?.stringValue = "\(record.link.id)"
                case "tcPortLink":
                    cellView.textField?.stringValue = "\(record.link.source.name) <=> \(record.link.target.name)"
                default:
                    cellView.textField?.stringValue = "-"
                }
            }
            return cellView
        }
        
        return nil
    }
    
    private func isPortAssigned(_ portId: UInt8) -> Bool {
        return model.isPortAssigned(portId: portId)
    }
    
    func dataChanged(_ data: [TDataRecord]?) {
        guard data == data else {
            return
        }
        DispatchQueue.main.async {
            self.m_firstPort = 0;
            self.m_secondPort = 0;
            self.cbxFirstPort.reset()
            self.cbxFirstPort.isEnabled = false
            self.cbxSecondPort.reset()
            self.cbxSecondPort.isEnabled = false
            self.tableView.isEnabled = false
            self.tableView.beginUpdates()
            data!.forEach {
                let r : TDataRecord = $0
                if r.type == .PortItem, !self.isPortAssigned(r.port.id) {
                    self.cbxFirstPort.addItem(withText: r.port.name, dataObject: r.port)
                    if self.m_firstPort == 0 {
                        self.m_firstPort = r.port.id // first cbx entry
                    }
                    self.cbxSecondPort.addItem(withText: r.port.name, dataObject: r.port)
                    if self.m_secondPort == 0 {
                        self.m_secondPort = r.port.id // first cbx entry
                    }
                }
            }
            self.tableView.reloadData()
            self.tableView.endUpdates()
            self.tableView.isEnabled = !(data?.isEmpty ?? false)
            if (self.tableView.isEnabled) {
                self.tableView.selectRowIndexes(IndexSet(//
                    integer: self.selectedRow > 0 ? self.selectedRow - 1 : 0),//
                                                byExtendingSelection: false)
            }
            if self.cbxFirstPort.numberOfItems > 0 {
                self.cbxFirstPort.selectItem(at: 0)
                self.cbxFirstPort.isEnabled = true
            }
            if self.cbxSecondPort.numberOfItems > 0 {
                self.cbxSecondPort.selectItem(at: 0)
                self.cbxSecondPort.isEnabled = true
            }
            self.pbCreateLink.isEnabled = (self.m_firstPort != self.m_secondPort)
            self.hideProgress()
        }
    }
    
    func createPopupMenu() -> NSMenu {
        let menu = NSMenu(title: "Select")
        menu.addItem(withTitle: "Delete", action: #selector(onDelete(_:)), keyEquivalent: "")
        return menu
    }
    
    @objc func onDelete(_ sender: Any?) {
        guard selectedRow >= 0, let record : TDataRecord = //
                model.getRecord(index: selectedRow, byType: TDataType.PortLink) else {
            return
        }
        if record.type != .PortLink {
            UITools.showMessage(message: "Can't delete non-port link item")
            return
        }
        hoverButton.isHidden = true
        if UITools.showQuestionDialog(self, "Delete this record?") {
            self.tableView.beginUpdates()
            model.removeRecord(index: selectedRow)
            showProgress()
            DispatchQueue.global(qos: .background).asyncAfter(//
                deadline: .now() + .milliseconds(100)) {
                UnlinkPorts(record.link.source.id, record.link.target.id)
                GetStatus()
            }
            self.tableView.endUpdates()
        }
    }
    
    func createHoverButton() -> NSButton {
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
