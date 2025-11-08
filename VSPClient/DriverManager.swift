// ********************************************************************
// VSPClient driver management API
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
import Foundation
import SystemExtensions
import AppKit
import IOKit
import Darwin
import SwiftUI
import Combine

// MARK: - Constants

let MAGIC_CONTROL: UInt64 = 0xBE6605250000
let MAX_SERIAL_PORTS: Int = 16
let MAX_PORT_LINKS: Int = 16
let MAX_PORT_NAME: Int = 64

// Bit manipulation helper (Swift doesn't have bit shifting in the same way)
func BIT(_ x: Int) -> UInt64 {
    return 1 << x
}

let TRACE_PORT_RX = BIT(16)
let TRACE_PORT_TX = BIT(17)
let TRACE_PORT_IO = BIT(18)
let CHECK_BAUD = BIT(19)
let CHECK_DATA_SIZE = BIT(20)
let CHECK_STOP_BITS = BIT(21)
let CHECK_PARITY = BIT(22)
let CHECK_FLOWCTRL = BIT(23)

@frozen
public struct TVSPSystemError {
    var system: Int32
    var sub: Int32
    var code: Int32
    
    init() {
        self.system = 0
        self.sub = 0
        self.code = 0
    }
    
    init (system: Int32, sub: Int32, code: Int32) {
        self.system = system
        self.sub = sub
        self.code = code
    }
}

/// Represents the lifecycle state of the DriverKit extension.
@objc enum DriverStatus: Int {
    case notLoaded
    case loading
    case loaded
    case unloading
    case unloaded
    case requiresUserApproval
    case willCompleteAfterReboot
    case failure
    case connected
    case disconnected
    case dataError
    case driverError
}

protocol DriverManagerObserver: AnyObject {
    func driverStatusDidChange(_ status: DriverStatus, code: UInt64, domain: String, message: String)
    func logMessageDidAvailable(_ message: String?)
}

protocol DriverDataObserver: AnyObject {
    func dataDidAvailable(_ data: TVSPControllerData?)
}

// -----------------------------------------------------------------
// VSP controller callback interface used in VSPController.cpp
// -----------------------------------------------------------------

@_silgen_name("VSPDriverConnected")
func VSPDriverConnected() {
    DriverManager.shared.driverConnected()
}

@_silgen_name("VSPDriverDisconnected")
func VSPDriverDisconnected() {
    DriverManager.shared.driverDisconnected()
}

@_silgen_name("VSPDataReady")
func VSPDataReady(_ args: TVSPControllerData?, numArgs: Int32) {

    // Sanity checks
    guard let args = args else {
        DriverManager.shared.dataError(0xbe000001,
                "Unexpected nil pointer in prarameter args.")
        return
    }
    guard numArgs != MemoryLayout<TVSPControllerData>.size else {
        DriverManager.shared.dataError(0xbe000002, "Unexpected data size.")
        return
    }
    
    DriverManager.shared.dataReady(args)
}

@_silgen_name("VSPLogMessage")
func VSPLogMessage(_ message: NSString?) {
    // Sanity checks
    guard let message = message else {
        DriverManager.shared.dataError(0xbe000010,
            "Unexpected nil pointer in prarameter message.")
        return
    }
    
    DriverManager.shared.logMessage(String(message))
}

@_silgen_name("VSPErrorOccured")
func VSPErrorOccured(_ code: UInt64, _ message: NSString?) {
    // Sanity checks
    guard let message = message else {
        DriverManager.shared.dataError(0xbe000011,
            "Unexpected nil pointer in prarameter message.")
        return
    }
    
    DriverManager.shared.driverErrorOccured(code, String(message))
}

final class DriverManager: NSObject, ObservableObject {
    
    // Singelton instance
    public static let shared: DriverManager = DriverManager()
    
    // The DriverKit extension bundle identifier
    private let driverIdentifier = "org.eof.tools.VSPDriver"
    
    // --
    private let errorDomain = "VSPDMErrorDomain"
    private var isDriverLoad: Bool = false
    private var isDriverUnload: Bool = false
    
    // UI-observable current status
    @Published @objc dynamic private(set) var status: DriverStatus = .notLoaded

    // Registered UI observers
    private var observers = NSHashTable<AnyObject>.weakObjects()
 
    // MARK: - Construction
    
    private override init() {
        super.init()
    }

    // MARK: - Observer registration

    public func addObserver(_ observer: DriverManagerObserver) {
        observers.add(observer)
    }

    public func addObserver(_ observer: DriverDataObserver) {
        observers.add(observer)
    }

    public func removeObserver(_ observer: DriverManagerObserver) {
        observers.remove(observer)
    }

    public func removeObserver(_ observer: DriverDataObserver) {
        observers.remove(observer)
    }

    private func notify(_ status: DriverStatus, code: UInt64, domain: String, message: String) {
        self.status = status
        for observer in observers.allObjects {
            (observer as? DriverManagerObserver)?
                .driverStatusDidChange(status, //
                    code: code, domain: errorDomain, message: message)
        }
    }

    // MARK: - Load DriverKit Extension
    
    public func loadDriver() {
        isDriverLoad = true
        isDriverUnload = false

        print("[VSPDRV] loadDriver() bundleID=\(driverIdentifier)")

        notify(.loading, code: 0, domain: errorDomain, //
               message: "Submitting activation request.")
        
        let activationRequest =
                OSSystemExtensionRequest.activationRequest(
                    forExtensionWithIdentifier: driverIdentifier,
                    queue: .main)
        activationRequest.delegate = self
        OSSystemExtensionManager.shared.submitRequest(activationRequest)
    }
    
    // MARK: - Unload DriverKit Extension
    
    public func unloadDriver() {
        isDriverLoad = false
        isDriverUnload = true
        
        notify(.unloading, code: 0, domain: errorDomain, //
               message: "Submitting deactivation request.")
   
        let deactivationRequest =
                OSSystemExtensionRequest.deactivationRequest(
                    forExtensionWithIdentifier: driverIdentifier,
                    queue: .main)
        deactivationRequest.delegate = self
        OSSystemExtensionManager.shared.submitRequest(deactivationRequest)
    }
    
    public func driverConnected() {
        notify(.connected, code: 0, domain: errorDomain, //
               message: "Driver successfully connected.")
    }
    
    public func driverDisconnected() {
        notify(.disconnected, code: 0, domain: errorDomain, //
               message: "Driver successfully disconnected.")
    }
    
    public func dataError(_ code: UInt64, _ message: String)
    {
        notify(.dataError, code: code, domain: errorDomain, //
               message: message)
    }

    public func driverErrorOccured(_ code: UInt64, _ message: String) {
        notify(.driverError, code: code, domain: errorDomain, //
               message: message)
    }
    
    public func dataReady(_ data: TVSPControllerData?) {
        for observer in observers.allObjects {
            (observer as? DriverDataObserver)?
                .dataDidAvailable(data)
        }
    }

    public func logMessage(_ message: String?) {
        for observer in observers.allObjects {
            (observer as? DriverManagerObserver)?
                .logMessageDidAvailable(message)
        }
    }
}

public extension TVSPControllerData {
    var userContextEnum: TVSPUserContext? {
        return TVSPUserContext(rawValue: self.context.rawValue)
    }

    var controlCommandEnum: TVSPControlCommand? {
        return TVSPControlCommand(rawValue: self.command.rawValue)
    }
}

// MARK: - OSSystemExtensionRequestDelegate

extension DriverManager: OSSystemExtensionRequestDelegate {
    
    internal func request(_ request: OSSystemExtensionRequest,
                 didFinishWithResult result: OSSystemExtensionRequest.Result) {

        switch result {
            case .completed:
                if isDriverLoad == true {
                    notify(.loaded, code: 0, domain: errorDomain,//
                           message: "Driver activated successfully")
                }
                else if isDriverUnload == true {
                    notify(.unloaded, code: 0, domain: errorDomain,//
                           message: "Driver deactivated successfully")
                    driverDisconnected();
                }
                else {
                    notify(.willCompleteAfterReboot, code: 0, domain: errorDomain, //
                           message: "Please reboot your computer to complete setup")
                }
            case .willCompleteAfterReboot:
                notify(.willCompleteAfterReboot, code: 0, domain: errorDomain, //
                       message: "A reboot is required to finalize this change")
            @unknown default:
                notify(.failure, code: 0, domain: errorDomain, //
                       message: "Unknown system state returned")
        }
    }

    internal func request(_ request: OSSystemExtensionRequest, didFailWithError error: Error) {
        let nsError = error as NSError
        notify(.failure,
               code: UInt64(nsError.code),
               domain: nsError.domain,
               message: error.localizedDescription)
    }

    internal func requestNeedsUserApproval(_ request: OSSystemExtensionRequest) {
        notify(.requiresUserApproval, code: 0, domain: errorDomain,
               message: "User must approve in System Settings " //
               + "→ Privacy & Security following driver extension:\n\nVirtual Serial Port Driver")
    }

    internal func request(
        _ request: OSSystemExtensionRequest,
        actionForReplacingExtension existing: OSSystemExtensionProperties,
        withExtension replacement: OSSystemExtensionProperties
    ) -> OSSystemExtensionRequest.ReplacementAction {
        let current = existing.description
        let newstr = replacement.description
        //print("[VSPDRV] Replacing extension: \(current) with \(newstr)")
        notify(.loading, code: 0, domain: errorDomain, //
               message: "Updating driver to a newer version.")
        return .replace
    }
}
