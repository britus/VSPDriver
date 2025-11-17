// ********************************************************************
// VSPClient User Interface
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
import Cocoa

// MARK: - Table View Data Source

extension TDataRecord: NSTableViewDataSource {
    func numberOfRows(in tableView: NSTableView) -> Int {
        return 0 // Will be handled by controller
    }
    
    func tableView(_ tableView: NSTableView, objectValueFor tableColumn: NSTableColumn?, row: Int) -> Any? {
        return nil // Will be handled by controller
    }
}

// MARK: - Table View Delegate

extension TDataRecord: NSTableViewDelegate {
    func tableView(_ tableView: NSTableView, viewFor tableColumn: NSTableColumn?, row: Int) -> NSView? {
        return nil // Will be handled by controller
    }
}

class TableView: NSTableView {

    override func draw(_ dirtyRect: NSRect) {
        super.draw(dirtyRect)
        // Drawing code here.
    }
    
}
