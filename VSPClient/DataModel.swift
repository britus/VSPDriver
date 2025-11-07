// ********************************************************************
// VSPClient User Interface
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
import Foundation
import AppKit
import Darwin
import SwiftUI
import Combine

@objc enum TDataType: UInt8 {
    case Unknown
    case PortItem
    case PortLink
}

struct TPortItem {
    var id: UInt8
    var name: String
    var flags: UInt64
    
    var description: String {
        return text()
    }

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
    
    func text() -> String {
        return "\(self.id) | \(self.name)"
    }
}

struct TPortLink {
    var id: UInt8
    var name: String
    var source: TPortItem
    var target: TPortItem
    var flags: UInt64

    var description: String {
        return "\(source.name) <=> \(target.name)"
    }

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

class Mutex {
    private let queue = DispatchQueue(label: "com.yourapp.mutex", attributes: .concurrent)
    
    func synchronized<T>(_ block: () throws -> T) rethrows -> T {
        return try queue.sync {
            try block()
        }
    }
    
    func synchronizedAsync(_ block: @escaping () -> Void) {
        queue.async {
            block()
        }
    }
}

class TDataRecord: NSObject, ObservableObject {
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

protocol DataModelObserver: AnyObject {
    func dataChanged(_ data: [TDataRecord]?)
}

class DataModel: NSObject {
 
    // Singelton instance
    public static let shared: DataModel = DataModel()
    
    // synchronized access
    let mutex = Mutex()

    // MARK: - Construction
    
    private override init() {}

    // Registered UI observers
    private var observers = NSHashTable<AnyObject>.weakObjects()
 
    // MARK: - Observer registration

    public func addObserver(_ observer: DataModelObserver) {
        observers.add(observer)
    }

    public func removeObserver(_ observer: DataModelObserver) {
        observers.remove(observer)
    }

    // data storage
    private var m_records: [TDataRecord] = []
    
    public func loadModel(_ settings: UserDefaults) -> Bool {
        //TODO: implement load from userDefaults
        return true;
    }
    
    public func saveModel(_ settings: UserDefaults) -> Void {
        //TODO: implement save to userDefaults
    }
    
    public func appendRecord(_ record: TDataRecord, notify: Bool = true) -> Void {
        let _ = mutex.synchronized {
            m_records.append(record)
        }
        if notify {
            notifyObservers()
        }
    }
    
    public func getRecord(index: Int, byType: TDataType) -> TDataRecord? {
        if index >= m_records.count {
            return nil
        }
        var _results: [TDataRecord] = []
        let _ = mutex.synchronized {
            self.m_records.forEach { (record) in
                if record.type == byType {
                    _results.append(record)
                }
            }
        }
        if _results.count == 0 || index >= _results.count || index < 0 {
            return nil
        }
        return _results[index]
    }
    
    public func recordCount(byType: TDataType = TDataType.Unknown) -> Int {
        var count : Int = 0
        let _ = mutex.synchronized {
            if byType == .Unknown {
                count = self.m_records.count
                return
            }
            self.m_records.forEach { (record) in
                if record.type == byType {
                    count += 1
                }
            }
        }
        return count
    }
    
    public var isEmpty: Bool {
        return m_records.isEmpty
    }
    
    public func removeAll(notyfy: Bool = true) -> Void {
        let _ = mutex.synchronized {
            self.m_records.removeAll()
        }
        if (notyfy) {
            notifyObservers()
        }
    }
    
    public func removeRecord(index: Int) -> Void {
        let _ = mutex.synchronized {
            self.m_records.remove(at: index)
        }
        notifyObservers()
    }
    
    public func removeRecord(_ record: TDataRecord) -> Void {
        if let index = m_records.firstIndex(of: record) {
            m_records.remove(at: index)
            notifyObservers()
        }
    }
    
    public func updated() -> Void {
        notifyObservers()
    }

    private func notifyObservers() {
        for observer in observers.allObjects {
            (observer as? DataModelObserver)?
                .dataChanged(m_records)
        }
    }
    
    public func isPortAssigned(portId: UInt8) -> Bool
    {
        var found: Bool = false
        m_records.forEach { (record) in
            if record.type == .PortLink {
                if record.link.source.id == portId || record.link.target.id == portId {
                    found = true
                    return
                }
            }
        }
        return found
    }
}
