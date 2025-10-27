//
//  DriverModel.swift
//  VCPClient
//
//  Created by Björn Eschrich on 26.10.25.
//

import Cocoa

@objc enum TDataType: UInt8 {
    case Unknown
    case PortItem
    case PortLink
}

struct TPortItem {
    var id: UInt8
    var name: String
    var flags: UInt64
    
    init(id: UInt8, name: String, flags: UInt64) {
        self.id = id
        self.name = name
        self.flags = flags
    }
    
    init () {
        self.id = 0
        self.name = ""
        self.flags = 0
    }
}

struct TPortLink {
    var id: UInt8
    var name: String
    var source: TPortItem
    var target: TPortItem
    var flags: UInt64
    
    init(id: UInt8, name: String, source: TPortItem, target: TPortItem, flags: UInt64) {
        self.id = id
        self.name = name
        self.source = source
        self.target = target
        self.flags = flags
    }
    
    init () {
        self.id = 0
        self.name = ""
        self.source = TPortItem(id: 0, name: "", flags: 0)
        self.target = TPortItem(id: 0, name: "", flags: 0)
        self.flags = 0
    }
}

class TDataRecord: NSObject {
    var type: TDataType = .Unknown
    var port: TPortItem = TPortItem(id: 0, name: "", flags: 0)
    var link: TPortLink = TPortLink(id: UInt8(0), name: "", //
                                    source: TPortItem(id: 0, name: "", flags: 0), //
                                    target: TPortItem(id: 0, name: "", flags: 0), //
                                    flags: 0)
    var flags: UInt64 = 0
    
    override init() {
        super.init()
    }
    
    convenience init(type: TDataType, port: TPortItem, link: TPortLink, flags: UInt64 = 0) {
        self.init()
        self.type = type
        self.port = port
        self.link = link
        self.flags = flags
    }
}

class DriverModel: NSObject {

    var m_records: [TDataRecord] = []
    
    public func loadModel(_ settings: UserDefaults) -> Bool {
        
         
        return true;
    }
    
    public func saveModel(_ settings: UserDefaults) -> Void {
        
        
    }

    public func appendRecord(_ record: TDataRecord) -> Void {
        m_records.append(record)
    }
    
    public func getRecord(index: Int) -> TDataRecord? {
        if index >= m_records.count {
            return nil
        }
        return m_records[index]
    }
    
    public var recordCount: Int {
        return m_records.count
    }
    
    public var isEmpty: Bool {
        return m_records.isEmpty
    }
    
    public func removeAll() -> Void {
        m_records.removeAll()
    }
    
    public func removeRecord(index: Int) -> Void {
        m_records.remove(at: index)
    }
    
    public func removeRecord(_ record: TDataRecord) -> Void {
        if let index = m_records.firstIndex(of: record) {
            m_records.remove(at: index)
        }
    }
}
