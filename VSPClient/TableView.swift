//
//  TableView.swift
//  VCPClient
//
//  Created by Björn Eschrich on 26.10.25.
//

import Cocoa

// MARK: - Extension for NSTableView DataSource and Delegate
extension TDataRecord {
    // Extension methods for table view interaction
    func displayValue(for column: Int) -> String {
        switch column {
        case 0:
            return type.rawValue.description
        case 1:
            switch type {
            case .PortItem:
                return port.name
            case .PortLink:
                return link.name
            default:
                return ""
            }
        case 2:
            switch type {
            case .PortItem:
                return port.id.description
            case .PortLink:
                return link.source.name + " → " + link.target.name
            default:
                return ""
            }
        case 3:
            switch type {
            case .PortItem:
                return port.flags.description
            case .PortLink:
                return link.flags.description
            default:
                return ""
            }
        default:
            return ""
        }
    }
    
    func displayValueForTypeColumn() -> String {
        switch type {
        case .Unknown: return "Unknown"
        case .PortItem: return "Port Item"
        case .PortLink: return "Port Link"
        }
    }
    
    func titleForColumn(_ column: Int) -> String {
        switch column {
        case 0: return "Type"
        case 1: return "Name"
        case 2: return "ID/Details"
        case 3: return "Flags"
        default: return ""
        }
    }
}

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
