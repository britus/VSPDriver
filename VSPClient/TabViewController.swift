//
//  TabViewController.swift
//  VCPClient
//
//  Created by Björn Eschrich on 26.10.25.
//

import Cocoa
import SwiftUI

class TabViewController: NSTabViewController {
    
    private let manager = DriverManager.shared
    private var isConnected: Bool = false
    
    override func viewDidLoad() {
        super.viewDidLoad()
        // hide stupid tab buttons
        self.tabStyle = .unspecified
        manager.addObserver(self)
        
        // C bridge async
        if (!IsDriverConnected()) {
            if (!ConnectDriver()) {
            }
        }
    }
    
    override func viewDidAppear() {
        super.viewDidAppear()
    }
    
    override func viewWillDisappear() {
        DisconnectDriver() // C bridge async
        manager.removeObserver(self)
        super.viewWillDisappear()
    }
    
    override func viewDidDisappear() {
        super.viewDidDisappear()
    }
    
    override func willPresentError(_ error: any Error) -> any Error {
        NSLog("willSelect: \(String(describing: error))")
        return error
    }
    
    override func tabView(_ tabView: NSTabView, willSelect tabViewItem: NSTabViewItem?) {
        NSLog("willSelect: \(String(describing: tabViewItem))")
    }
    
    override func tabView(_ tabView: NSTabView, didSelect tabViewItem: NSTabViewItem?) {
        NSLog("didSelect: \(String(describing: tabViewItem))")
    }
    
    @objc func willLoadDriver() {
        //
    }
    
    @objc func needsUserApproval() {
        UITools.showBasicNotification(body: "VSP controller wait for your approval.")
    }
    
    @objc func didFinish(withResult code: UInt64, message: String) {
        UITools.showBasicNotification(body: "Error \(code): \(message).")
    }
    
    @objc func didFail(withError code: UInt64, message: String) {
        UITools.showBasicNotification(body: "Error \(code): \(message).")
    }
    
    @objc func willUnloadDriver() {
        DisconnectDriver()
    }
    
    @objc func didUnload(withStatus code: UInt64, message: String) {
        UITools.showBasicNotification(body: "Error \(code): \(message).")
    }
    
    @objc func controllerConnected() {
        if (!isConnected) {
            UITools.showBasicNotification(body: "VSP controller is up and running")
            view.isHidden = false
            isConnected = true
            GetStatus()
        }
    }
    
    @objc func controllerDisconnected() {
        if (isConnected) {
            UITools.showBasicNotification(body: "VSP controller disconnected")
            isConnected = false
        }
    }
    
    @objc func dataError(withCode code: UInt64, message: String) {
        UITools.showBasicNotification(body: "Error \(code): \(message).")
    }
}

extension TabViewController: DriverManagerObserver {
    func driverStatusDidChange(_ status: DriverStatus, code: UInt64, domain: String, message: String) {
        switch status {
            case .notLoaded:
                UITools.showBasicNotification(body: message)
                break
            case .loading:
                willLoadDriver()
                break
            case .loaded:
                didFinish(withResult: code, message: message)
                break
            case .unloading:
                willUnloadDriver()
                break
            case .unloaded:
                didUnload(withStatus: code, message: message)
                break
            case .failure:
                didFail(withError: code, message: message)
                break
            case .requiresUserApproval:
                needsUserApproval()
                break
            case .willCompleteAfterReboot:
                didFail(withError: code, message: message)
                break
            case .connected:
                controllerConnected()
                break
            case .disconnected:
                controllerDisconnected()
                break
            case .dataError:
                dataError(withCode: code, message: message)
                break
        }
    }
    
    func dataReady(_ data: TVSPControllerData?)
    {
        UITools.showBasicNotification(body: "Data ready: \(String(describing: data))")
    }
}
