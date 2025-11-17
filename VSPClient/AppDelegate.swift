// ********************************************************************
// VSPClient User Interface
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
import Cocoa
import AppKit
import SwiftUI
import StoreKit

@main
class AppDelegate: NSObject, NSApplicationDelegate {
    
    static public var shared: AppDelegate {
        return NSApp.delegate as! AppDelegate
    }
    
    static public var window: NSWindow? {
        return NSApp.mainWindow
    }

    static public var viewController: NSViewController?
    
    static public var isRestricted: Bool {
        get {
            return PSKManager.shared.isRestricted
        }
    }
    
    @IBOutlet weak var m_likeOrUnlike: NSMenuItem!
    @IBOutlet weak var m_about: NSMenuItem!
    @IBOutlet weak var m_driverInstall: NSMenuItem!
    @IBOutlet weak var m_driverUninstall: NSMenuItem!
    
    @IBAction func onInstallDriver(_ sender: Any) {
        DriverManager.shared.loadDriver()
    }
    
    @IBAction func onUninstallDriver(_ sender: Any) {
        DriverManager.shared.unloadDriver()
    }
    
    @IBAction func onShutdownDriver(_ sender: Any) {
        //DriverManager.shared.unloadDriver()
        if UITools.showQuestionDialog(self, "Do you want to continue?", info:
            "Before to continue, open Terminal window and " +
            "enter following commands:\n\n" +
            "systemextensionsctl developer on\n" +
            "systemextensionsctl reset\n\n" +
            "Be careful! This command will remove ALL of your system extensions!\n" +
            "If you have more than Virtual Serial Port Driver, you can " +
            "Install all other system extensions manually.") {
            ShutdownDriver();
            NSApp.terminate(nil)
        }
    }
    
    @IBAction func onLikeOrUnlike(_ sender: NSMenuItem) {
        if let vc = AppDelegate.viewController {
            PSKManager.shared.review(vc)
        }
    }
    
    func applicationDidFinishLaunching(_ aNotification: Notification) {
        // Register for window restoration
        NSApp.setActivationPolicy(.regular)
        // Disable assistant for this app
        UserDefaults.standard.set(false, forKey: "NSAssistantEnabled")
    }
    
    func applicationWillTerminate(_ aNotification: Notification) {
        if IsDriverConnected() {
            DisconnectDriver()
        }
    }
    
    func applicationShouldTerminate(_ sender: NSApplication) -> NSApplication.TerminateReply {
        // Optionally perform cleanup before termination
        return .terminateNow
    }
    
    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool {
        // Terminate app when the last window is closed
        return true
    }
    
    func applicationSupportsSecureRestorableState(_ app: NSApplication) -> Bool {
        return true
    }
}
