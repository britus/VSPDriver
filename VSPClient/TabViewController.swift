// ********************************************************************
// VSPClient User Interface
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
import Cocoa
import SwiftUI

class TabViewController: NSTabViewController {
    
    private var isConnected: Bool = false
    private let manager = DriverManager.shared
    private var model : DataModel = DataModel.shared
    private var page : NSTabViewItem?

    public var isEnabled : Bool {
        get {
            return pageEnabled()
        }
        set {
            setPageEnabled(newValue)
        }
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        // hide stupid tab buttons
        self.tabStyle = .unspecified
        // set main view controller
        AppDelegate.viewController = self
        // query purchase state
        PSKManager.shared.query()
    }
    
    override func viewDidAppear() {
        super.viewDidAppear()
        manager.addObserver(self)
    }
    
    override func viewWillDisappear() {
        manager.removeObserver(self)
        super.viewWillDisappear()
    }
    
    override func viewDidDisappear() {
        super.viewDidDisappear()
    }
    
    public func pageEnabled() -> Bool {
        guard let pg = page else {
            return false
        }
        guard let tv = pg.tabView else {
            return false
        }
        return tv.isViewEnabled
    }

    public func setPageEnabled(_ enabled: Bool) {
        guard let pg = page else {
            return
        }
        guard let tv = pg.tabView else {
            return
        }
        tv.setControlsEnabled(enabled)
    }

    override func tabView(_ tabView: NSTabView, didSelect tabViewItem: NSTabViewItem?) {
        page = tabViewItem
        setPageEnabled(false)
        if (IsDriverConnected()) {
            showProgress()
            DispatchQueue.global(qos: .background).asyncAfter(//
                                 deadline: .now() + .milliseconds(100)) {
                GetStatus()
            }
        }
    }
}

extension TabViewController: DriverDataObserver {
    
    func dataError(withCode code: UInt64, message: String) {
        if code > 0 {
            UITools.showMessage(message: "VSP Error \(String(format: "0x%llx", code)):\n\n\(message).")
        } else {
            UITools.showMessage(message: "\(message).")
        }
    }
    
    func dataDidAvailable(_ data: TVSPControllerData?)
    {
        //UITools.showBasicNotification(body: "Data ready: \(String(describing: data))")
        if (data == nil) {
            UITools.showMessage(message: "Invalid data detected.")
            return
        }
        
        DispatchQueue.main.async {
            self.setPageEnabled(!AppDelegate.isRestricted)
            self.hideProgress()
        }

        let input : TVSPControllerData = data!

        if input.context == TVSPUserContext.error {
            switch (input.status.flags) {
            case 0xfa000001:
                switch (input.status.code) {
                    case 0xe0000001:
                        dataError(withCode: UInt64(input.status.code), //
                                  message: "Same ports cannot be linked together.")
                        break
                    case 0xe00002d5:
                        dataError(withCode: UInt64(input.status.code), //
                                  message: "One of the ports is already in use.")
                        break
                    case 0xe00002c2:
                        dataError(withCode: UInt64(input.status.code), //
                                  message:  "Invalid parameter detected.")
                        break
                    case 0xe00002db:
                        dataError(withCode: UInt64(input.status.code), //
                                  message: "Maximum of available ports reached.")
                        break
                    default:
                        dataError(withCode: UInt64(input.status.code), //
                                  message: "VSP Error \(String(format: "0x%llx", input.status.code))")
                        break
                }
                break
            case 0xfa000000:
                dataError(withCode: UInt64(input.status.code), //
                          message: "VSP Error \(String(format: "0x%llx", input.status.code))")
                break
            default:
                if input.status.code > 0 {
                    dataError(withCode: UInt64(input.status.code), //
                             message: "VSP Error \(String(format: "0x%llx", input.status.code))")
                }
                break
            }
            return
        }
        
        if input.context == TVSPUserContext.result {
            var isModified : Bool = false

            // reset current data model
            if input.command == TVSPControlCommand.removePort //
                || input.command == TVSPControlCommand.unlinkPorts //
                || input.command == TVSPControlCommand.createPort //
                || input.command == TVSPControlCommand.linkPorts //
                || input.command == TVSPControlCommand.getStatus {
                model.removeAll(notyfy: false)
                if !isModified { isModified = true }
            }
                        
            for i in 0..<input.ports.count {
                // get driver structure object (Objective-C)
                let port : TVSPPortListItem = input.ports.items[Int(i)]
                // create and apend data record to model
                let record : TDataRecord = TDataRecord()
                record.type = .PortItem
                record.flags = 0x00
                record.port = TPortItem(id: port.portId, name: port.name, flags: port.flags)
                model.appendRecord(record, notify: false);
                if !isModified { isModified = true }
            }

            for i in 0..<input.links.count {
                // get port link byte encoded
                let link = input.links.links[Int(i)].uint64Value
                let _lid = (link >> 16) & 0x000000ff;
                let _src = (link >> 8) & 0x000000ff;
                let _tgt = link & 0x000000ff;
                // create and append data record to model
                let record : TDataRecord = TDataRecord()
                record.type = .PortLink
                record.flags = 0x00
                record.link = TPortLink(id: UInt8(_lid),
                            name: "Link: Port \(_src) <=> Port \(_tgt)", //
                            source: TPortItem(id: UInt8(_src), name: "Port \(_src)", flags: 0), //
                            target: TPortItem(id: UInt8(_tgt), name: "Port \(_tgt)", flags: 0), //
                            flags: 0x00)
                model.appendRecord(record, notify: false);
                if !isModified { isModified = true }
            }
            
            if (isModified) {
                model.updated()
            }
        }
    }
}
