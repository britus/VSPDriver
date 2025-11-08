// ********************************************************************
// VSPClient User Interface
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
import Cocoa
import SwiftUI

class WindowController: NSWindowController {
    @IBOutlet weak var tbSerialPorts: NSToolbarItem!
    @IBOutlet weak var tbSerialTest: NSToolbarItem!
    @IBOutlet weak var tbPortLinks: NSToolbarItem!
    @IBOutlet weak var tbMessages: NSToolbarItem!
    @IBOutlet weak var toolBar: NSToolbar!
 
    private let manager = DriverManager.shared
    private weak var tabView: TabViewController!
    
    private static let initialSize = NSSize(width: 620, height: 450)
    
    // Window size constants Size: 615 × 622
    private let minWindowSize = WindowController.initialSize
    private let preferredWindowSize = WindowController.initialSize
    private var progress : NSProgressIndicator? = nil
    
    override func windowWillLoad() {
        manager.addObserver(self)
    }

    override func windowDidLoad() {
        let vc = self.contentViewController
        tabView = (vc as! TabViewController)
        updateButtons(false)
        configureWindow()
        showProgress()

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
        tabView.selectedTabViewItemIndex = 0
    }
    
    @IBAction func onPortLinks(_ sender: Any) {
        tabView.selectedTabViewItemIndex = 1
    }
    
    @IBAction func onSMessageView(_ sender: Any) {
        tabView.selectedTabViewItemIndex = 2
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
            self.tbMessages.isEnabled = state
            self.tbSerialTest.isEnabled = state
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

// MARK: - DriverManagerObserver

extension WindowController: DriverManagerObserver {
    func willLoadDriver() {
        //NSLog("Loading driver extension...")
    }
    
    func didFinish(withResult code: UInt64, message: String) {
        hideProgress()
        
        if code > 0 {
            tabView.isEnabled = false
            UITools.showMessage(
                message: "Error 0x\(String(code, radix: 16)): \(message).",
                withCompletion: { NSApp.terminate(self) })
        } else {
            UITools.showNotification(body: "\(message).")
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
        hideProgress()

        if code > 0 {
            tabView.isEnabled = false
            UITools.showMessage(
                message: "VSP Error 0x\(String(code, radix: 16)):\n\n\(message)",
                withCompletion: { NSApp.terminate(self) })
        } else {
            UITools.showMessage(message: message)
        }
    }
    
    func willUnloadDriver() {
        DisconnectDriver()
    }
    
    func didUnload(withStatus code: UInt64, message: String) {
        tabView.isEnabled = false
        if code > 0 {
            UITools.showMessage(
                message: "Error 0x\(String(code, radix: 16)): \(message).")
        } else {
            UITools.showNotification(body: message)
        }
    }
    
    func controllerConnected() {
        hideProgress()
        tabView.isEnabled = true;
        updateButtons(tabView.isEnabled)
        UITools.showNotification(body: "VSP Driver connected")
        
        // C bridge async
        DispatchQueue.global(qos: .background).asyncAfter(//
                deadline: .now() + .milliseconds(100)) {
            GetStatus()
        }
    }
    
    func controllerDisconnected() {
        hideProgress()
        tabView.isEnabled = false;
        updateButtons(tabView.isEnabled)
        UITools.showNotification(body: "VSP Driver disconnected")
    }
    
    func logMessageDidAvailable(_ message: String?) {
        //guard let msg = message, !msg.isEmpty else {
        //    return
        //}
        // TODO: Make tool window with message text view
        // NSLog("\(msg)\n")
    }
    
    func driverStatusDidChange(_ status: DriverStatus, code: UInt64, domain: String, message: String) {
        switch status {
            case .notLoaded:
                DispatchQueue.main.async {
                    UITools.showNotification(body: message)
                }
                break
            case .loading:
                DispatchQueue.main.async {
                    self.willLoadDriver()
                }
                break
            case .loaded:
                DispatchQueue.main.async {
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
                    self.didUnload(withStatus: code, message: message)
                }
                break
            case .failure:
                DispatchQueue.main.async {
                    self.didFail(withError: code, message: message)
                }
                break
            case .requiresUserApproval:
                UITools.showMessage(message: message) {
                    if !IsDriverConnected() {
                        UITools.showMessage(message:
                            "Unable to connect VSP driver.\nPlease reboot your computer.") {
                            NSApp.terminate(self)
                        }
                    }
                }
                break
            case .willCompleteAfterReboot:
                UITools.showMessage(message: message) {
                    NSApp.terminate(self)
                }
                break
            case .connected:
                DispatchQueue.main.async {
                    self.controllerConnected()
                }
                break
            case .disconnected:
                DispatchQueue.main.async {
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
