// ********************************************************************
// VSPClient User Interface
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
import Cocoa
import SwiftUI

let VIEW_CONTROLLER_ID = "TestSerialViewController"

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
    internal var resizePopupTimer: Timer?
    internal var lastShownSize: NSSize?
    
    override func windowDidLoad() {
        NSLog("\(String(describing: self.contentViewController))")
        tabViewController = ((self.contentViewController as! NSTabViewController) as! TabViewController)
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
        
        // Set maximum size (optional)
        // window.maxSize = NSSize(width: 2000, height: 1500)
        
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
    
    // Method to add resize indicator view
    private func setupResizeIndicator() {
        guard let window = self.window else { return }
        
        // Create a label to show dimensions
        let dimensionsLabel = NSTextField()
        dimensionsLabel.isEditable = false
        dimensionsLabel.isBordered = false
        dimensionsLabel.backgroundColor = NSColor.black.withAlphaComponent(0.7)
        dimensionsLabel.textColor = NSColor.white
        dimensionsLabel.font = NSFont.systemFont(ofSize: 12)
        dimensionsLabel.stringValue = "Size: \(Int(preferredWindowSize.width)) × \(Int(preferredWindowSize.height))"
        dimensionsLabel.translatesAutoresizingMaskIntoConstraints = false
        
        // Add to window content view
        if let contentView = window.contentView {
            contentView.addSubview(dimensionsLabel)
            
            // Position the label in bottom-right corner
            NSLayoutConstraint.activate([
                dimensionsLabel.trailingAnchor.constraint(equalTo: contentView.trailingAnchor, constant: -10),
                dimensionsLabel.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: -10),
                dimensionsLabel.widthAnchor.constraint(equalToConstant: 120),
                dimensionsLabel.heightAnchor.constraint(equalToConstant: 20)
            ])
            
            // Store reference to update later
            window.delegate = self
        }
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
    
    // Method to update dimensions label
    private func updateDimensionsLabel() {
        guard let window = self.window,
              let contentView = window.contentView else { return }
        
        // Find or create the dimensions label
        let dimensionsLabel = contentView.subviews.first { view in
            return view is NSTextField && (view as? NSTextField)?.isEditable == false
        } as? NSTextField
        
        // Update the label with current dimensions
        let currentSize = window.frame.size
        if let label = dimensionsLabel {
            label.stringValue = "Size: \(Int(currentSize.width)) × \(Int(currentSize.height))"
        }
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
    
    // Method to show resize popup with delay (prevents spamming)
    public func showResizePopupWithDelay() {
        // Cancel any existing timer
        resizePopupTimer?.invalidate()
        // Create a new timer to delay the popup
        resizePopupTimer = Timer.scheduledTimer(withTimeInterval: 0.5, repeats: false) { _ in
            self.showResizePopup()
        }
    }
    
    private func showResizePopup() {
        guard let window = self.window else { return }
        
        let currentSize = window.frame.size
        let width = Int(currentSize.width)
        let height = Int(currentSize.height)
        
        // Only show popup if size has actually changed significantly
        let currentSizeInt = NSSize(width: width, height: height)
        
        if lastShownSize == nil ||
           abs(lastShownSize!.width - currentSize.width) > 10 ||
           abs(lastShownSize!.height - currentSize.height) > 10 {
            
            let alert = NSAlert()
            alert.messageText = "Window Resized"
            alert.informativeText = "Size: \(Int(currentSize.width)) × \(Int(currentSize.height))"
            alert.alertStyle = .informational
            alert.addButton(withTitle: "OK")
            
            // Add copy button for dimensions
            alert.addButton(withTitle: "Copy Size")
            
            let response = alert.runModal()
            
            if response == .alertSecondButtonReturn {
                // Copy to clipboard
                let pasteboard = NSPasteboard.general
                pasteboard.clearContents()
                pasteboard.setString("Size: \(Int(currentSize.width)) × \(Int(currentSize.height))", forType: .string)
            }
            
            lastShownSize = currentSizeInt
        }
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
        // Show resize popup with delay to prevent spam
        // showResizePopupWithDelay()
    }
    
    func windowDidMove(_ notification: Notification) {
        // Handle window movement if needed
    }
    
    func windowWillClose(_ notification: Notification) {
        // Cancel timer when window closes
        resizePopupTimer?.invalidate()
        DisconnectDriver()
    }
}

extension WindowController: DriverManagerObserver {
    func willLoadDriver() {
        //
    }
    
    func didFinish(withResult code: UInt64, message: String) {
        if code > 0 {
            UITools.showMessage(message: "Error 0x\(String(code, radix: 16)): \(message).")
        } else {
            UITools.showBasicNotification(body: "\(message).")
            // C bridge async
            DispatchQueue.global(qos: .background).asyncAfter(//
                deadline: .now() + .milliseconds(100)) {
                if (!ConnectDriver()) {
                    UITools.showMessage(message: "\(message),\nbut failed to connect driver." //
                                        + "\nPlease reboot your computer to complete the setup.")
                    self.updateButtons(false)
                }
            }
        }
    }
    
    func didFail(withError code: UInt64, message: String) {
        if code > 0 {
            UITools.showMessage(message: "Error 0x\(String(code, radix: 16)): \(message).")
        } else {
            UITools.showMessage(message: "\(message).")
        }
    }
    
    func willUnloadDriver() {
        DisconnectDriver()
    }
    
    func didUnload(withStatus code: UInt64, message: String) {
        if code > 0 {
            UITools.showMessage(message: "Error 0x\(String(code, radix: 16)): \(message).")
        } else {
            UITools.showNotificationWithBadge(body: message, badgeCount: 1)
        }
    }
    
    func controllerConnected() {
        DispatchQueue.main.async {
            UITools.showNotificationWithBadge(body: "VSP Driver connected", badgeCount: 1)
            GetStatus()
        }
    }
    
    func controllerDisconnected() {
        UITools.showNotificationWithBadge(body: "VSP Driver disconnected", badgeCount: 1)
    }
    
    func logMessageDidAvailable(_ message: String?) {
        guard let msg = message, !msg.isEmpty else {
            return
        }
        NSLog("˜\(msg)\n")
    }
    
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
                UITools.showMessage(message: message)
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
                didFail(withError: code, message: message)
                break
            default:
                break
        }
    }
}

// Extension to handle window management
extension NSApplication {
    static func showViewControllerFromStoryboardInExistingWindow() {
        // Get the storyboard
        let storyboard = NSStoryboard(name: "Main", bundle: nil)
        
        // Instantiate the view controller from storyboard
        let storyboardIdentifier = VIEW_CONTROLLER_ID
        
        guard let viewController = storyboard.instantiateController(withIdentifier: storyboardIdentifier) as? NSViewController else {
            print("Failed to instantiate view controller from storyboard")
            return
        }
        
        // Get existing window or create new one
        let windows = NSApp.windows
        var targetWindow: NSWindow?
        
        if !windows.isEmpty {
            // Use existing window or create new one
            targetWindow = windows.first
        } else {
            // Create new window if no existing ones
            targetWindow = NSWindow(
                contentRect: NSMakeRect(100, 100, 400, 300),
                styleMask: [.titled, .closable, .resizable],
                backing: .buffered,
                defer: false
            )
            targetWindow?.title = "Storyboard Window"
        }
        
        // Set up the window if it doesn't exist
        if targetWindow == nil {
            targetWindow = NSWindow(
                contentRect: NSMakeRect(100, 100, 400, 300),
                styleMask: [.titled, .closable, .resizable],
                backing: .buffered,
                defer: false
            )
            targetWindow?.title = "Storyboard Window"
        }
        
        // Set the view controller's view as content
        targetWindow?.contentView = viewController.view
        
        // Make sure the window is properly configured
        targetWindow?.makeKeyAndOrderFront(nil)
        
        // Ensure the view controller is properly set up
        if let window = targetWindow {
            window.makeKeyAndOrderFront(nil)
        }
    }
}
