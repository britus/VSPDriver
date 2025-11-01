// ********************************************************************
// VSPClient User Interface
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
import Cocoa
import SwiftUI

let SP_TEST_VIEW_ID = "TestSerialViewController"

class WindowController: NSWindowController {
    @IBOutlet weak var tbSerialPorts: NSToolbarItem!
    @IBOutlet weak var tbSerialTest: NSToolbarItem!
    @IBOutlet weak var tbPortLinks: NSToolbarItem!
    @IBOutlet weak var tbMessages: NSToolbarItem!
    @IBOutlet weak var toolBar: NSToolbar!
 
    private let manager = DriverManager.shared
    private weak var tabViewController: TabViewController!
    
    private static let initialSize = NSSize(width: 620, height: 450)
    
    // Window size constants Size: 615 × 622
    private let minWindowSize = WindowController.initialSize
    private let preferredWindowSize = WindowController.initialSize
    
    override func windowDidLoad() {
        NSLog("\(String(describing: self.contentViewController))")
        tabViewController = ((self.contentViewController //
                              as! NSTabViewController) as! TabViewController)
        tabViewController?.view.isHidden = true
        manager.addObserver(self)
        
        // Configure window appearance and behavior
        configureWindow()
 
        // Request permission first (required)
        UITools.requestNotificationPermission { granted in
            // this closure run always
            if (!IsDriverConnected()) {
                // C bridge async
                DispatchQueue.global(qos: .background).asyncAfter(//
                    deadline: .now() + .milliseconds(100)) {
                    if (!ConnectDriver()) {
                        self.manager.loadDriver()
                    }
                }
            }
        }
    }
    
    override func windowTitle(forDocumentDisplayName displayName: String) -> String {
        let v : String = UITools.applicationVersion()
        let b : String = UITools.applicationBuild()
        return "VSP Controller \(v).\(b)"
    }
    
    @IBAction func onSerialPorts(_ sender: Any) {
        tabViewController.selectedTabViewItemIndex = 0
    }
    
    @IBAction func onPortLinks(_ sender: Any) {
        tabViewController.selectedTabViewItemIndex = 1
    }
    
    @IBAction func onSMessageView(_ sender: Any) {
        tabViewController.selectedTabViewItemIndex = 2
    }
    
    @IBAction func onSerialTest(_ sender: Any) {
        //tabViewController.selectedTabViewItemIndex = 3
    }
    
    private func setupWindowSizeConstraints() {
        guard let window = self.window else { return }
        
        // Set minimum size
        window.minSize = minWindowSize
         
        // Prevent window from being resized below minimum
        window.isRestorable = true
    }
    
    private func centerWindowOnScreen() {
        guard let window = self.window else { return }
        
        // Use the current screen's visible frame
        let screenFrame = NSScreen.main?.visibleFrame ?? NSRect.zero
        
        // Calculate centered position
        let windowWidth = preferredWindowSize.width
        let windowHeight = preferredWindowSize.height
        
        let centerX = (screenFrame.width - windowWidth) / 2
        let centerY = (screenFrame.height - windowHeight) / 2
        
        // Set the window frame with animation
        let newFrame = NSRect(
            x: centerX,
            y: centerY,
            width: windowWidth,
            height: windowHeight
        )
        
        window.setFrame(newFrame, display: true, animate: true)
    }
    
    private func configureWindowAppearance() {
        guard let window = self.window else { return }
        
        // Set window style mask
        window.styleMask = [
            .titled,
            .closable,
            .resizable,
            .miniaturizable
        ]
        
        // Set window level
        window.level = .normal
        
        // Enable full size content view (for modern macOS)
        window.contentView?.wantsLayer = true
        
        // Set background color
        window.backgroundColor = NSColor.windowBackgroundColor
        
        // Handle window resizing constraints
        window.delegate = self
        
        // Add a resize indicator view to show dimensions (optional)
        // setupResizeIndicator()
    }
    
    // Method to center window on multiple screens (if needed)
    func centerWindowOnMainScreen() {
        guard let window = self.window else { return }
        
        // Get main screen
        let mainScreen = NSScreen.main ?? NSScreen.screens.first!
        
        // Get visible frame (excluding menu bar and dock)
        let screenVisibleFrame = mainScreen.visibleFrame
        
        // Calculate center position
        let windowWidth = preferredWindowSize.width
        let windowHeight = preferredWindowSize.height
        
        let centerX = (screenVisibleFrame.width - windowWidth) / 2
        let centerY = (screenVisibleFrame.height - windowHeight) / 2
        
        // Set window frame
        let newFrame = NSRect(
            x: screenVisibleFrame.origin.x + centerX,
            y: screenVisibleFrame.origin.y + centerY,
            width: windowWidth,
            height: windowHeight
        )
        
        window.setFrame(newFrame, display: true)
    }
    
    internal func updateButtons(_ state: Bool)
    {
        DispatchQueue.main.async {
            self.tbPortLinks.isEnabled = state
            self.tbSerialPorts.isEnabled = state
            self.tabViewController.tabView.isHidden = !state
        }
    }
    
    private func configureWindow() {
        guard let window = self.window else {
            return
        }
        
        let n : String = UITools.applicationName()
        let v : String = UITools.applicationVersion()
        let b : String = UITools.applicationBuild()
        window.title = "\(n)"
        window.subtitle = "\(v).\(b)"

        // Set up minimum size constraints
        setupWindowSizeConstraints()
        // Center window on screen
        centerWindowOnScreen()
        // Configure additional window properties
        configureWindowAppearance()
    }
}

// MARK: - NSWindowDelegate Implementation for Enhanced Version

extension WindowController: NSWindowDelegate {
    
    func windowWillResize(_ window: NSWindow, to newFrameSize: NSSize) -> NSSize {
        let constrainedWidth = max(newFrameSize.width, WindowController.initialSize.width)
        let constrainedHeight = max(newFrameSize.height, WindowController.initialSize.height)
        return NSSize(width: constrainedWidth, height: constrainedHeight)
    }
    
    func windowDidResize(_ notification: Notification) {
    }
    
    func windowDidMove(_ notification: Notification) {
    }
    
    func windowWillClose(_ notification: Notification) {
        DisconnectDriver()
    }
}

extension WindowController: DriverManagerObserver {
    func willLoadDriver() {
        NSLog("Loading driver extension...")
    }
    
    func didFinish(withResult code: UInt64, message: String) {
        if code > 0 {
            UITools.showMessage(message: //
                "Error 0x\(String(code, radix: 16)): \(message).")
        } else {
            UITools.showBasicNotification(body: "\(message).")
            // C bridge async
            DispatchQueue.global(qos: .background).asyncAfter(//
                deadline: .now() + .milliseconds(100)) {
                if (!ConnectDriver()) {
                    UITools.showMessage(message://
                        "\(message),\nbut failed to connect driver." //
                        + "\nPlease reboot your computer to complete the setup.")
                    self.updateButtons(false)
                }
            }
        }
    }
    
    func didFail(withError code: UInt64, message: String) {
        if code > 0 {
            UITools.showMessage(message:
                "Error 0x\(String(code, radix: 16)): \(message).")
        } else {
            UITools.showMessage(message: "\(message).")
        }
    }
    
    func willUnloadDriver() {
        DisconnectDriver()
    }
    
    func didUnload(withStatus code: UInt64, message: String) {
        if code > 0 {
            UITools.showMessage(message:
                "Error 0x\(String(code, radix: 16)): \(message).")
        } else {
            UITools.showNotificationWithBadge(body: message, badgeCount: 1)
        }
    }
    
    func controllerConnected() {
        DispatchQueue.main.async {
            UITools.showNotificationWithBadge(
                body: "VSP Driver connected", badgeCount: 1)
            GetStatus()
        }
    }
    
    func controllerDisconnected() {
        UITools.showNotificationWithBadge(
            body: "VSP Driver disconnected", badgeCount: 1)
    }
    
    func logMessageDidAvailable(_ message: String?) {
        guard let msg = message, !msg.isEmpty else {
            return
        }
        // TODO: Make tool window with message text view
        NSLog("\(msg)\n")
    }
    
    func driverStatusDidChange(_ status: DriverStatus, code: UInt64, domain: String, message: String) {
        switch status {
            case .notLoaded:
                DispatchQueue.main.async {
                    self.tabViewController?.view.isHidden = true
                    UITools.showBasicNotification(body: message)
                }
                break
            case .loading:
                DispatchQueue.main.async {
                    self.willLoadDriver()
                }
                break
            case .loaded:
                DispatchQueue.main.async {
                    self.tabViewController?.view.isHidden = true
                    self.didFinish(withResult: code, message: message)
                }
                break
            case .unloading:
                DispatchQueue.main.async {
                    self.willUnloadDriver()
                }
                break
            case .unloaded:
                DispatchQueue.main.async {
                    self.tabViewController?.view.isHidden = true
                    self.didUnload(withStatus: code, message: message)
                }
                break
            case .failure:
                DispatchQueue.main.async {
                    self.tabViewController?.view.isHidden = true
                    self.didFail(withError: code, message: message)
                }
                break
            case .requiresUserApproval:
                DispatchQueue.main.async {
                    self.tabViewController?.view.isHidden = true
                    UITools.showMessage(message: message)
                }
                break
            case .willCompleteAfterReboot:
                DispatchQueue.main.async {
                    self.tabViewController?.view.isHidden = true
                    self.didFail(withError: code, message: message)
                }
                break
            case .connected:
                DispatchQueue.main.async {
                    self.tabViewController?.view.isHidden = false
                    self.controllerConnected()
                }
                break
            case .disconnected:
                DispatchQueue.main.async {
                    self.tabViewController?.view.isHidden = true
                    self.controllerDisconnected()
                }
                break
            case .dataError:
                DispatchQueue.main.async {
                    self.didFail(withError: code, message: message)
                }
                break
            default:
                break
        }
    }
}
