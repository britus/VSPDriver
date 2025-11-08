// ********************************************************************
// VSPClient User Interface
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
import Cocoa
import AppKit
import SwiftUI

@main
class AppDelegate: NSObject, NSApplicationDelegate {
    
    static var shared: AppDelegate {
        return NSApp.delegate as! AppDelegate
    }
    
    static var window: NSWindow? {
        return NSApp.mainWindow
    }
    
    @IBOutlet public weak var m_driverInstall: NSMenuItem!
    @IBOutlet public weak var m_driverUninstall: NSMenuItem!
    
    @IBAction func onInstallDriver(_ sender: Any) {
        DriverManager.shared.loadDriver()
    }
    
    @IBAction func onUninstallDriver(_ sender: Any) {
        DriverManager.shared.unloadDriver()
    }
    
    @IBAction func onShutdownDriver(_ sender: Any) {
        //DriverManager.shared.unloadDriver()
        UITools.showMessage(message:
            "Before to continue, open Terminal window and\n" +
            "enter following commands:\n\n" +
            "systemextensionsctl developer on\n" +
            "systemextensionsctl reset.\n\n" +
            "Be careful! This command will remove ALL of your\nsystem extensions! " +
            "If you have more than\nVirtual Serial Port Driver, you can " +
            "Install all\nother system extensions manually.", info: nil) //
        {
            ShutdownDriver();
            NSApp.terminate(nil)
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
