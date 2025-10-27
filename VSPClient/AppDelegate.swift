//
//  AppDelegate.swift
//  VCPClient
//
//  Created by Björn Eschrich on 25.10.25.
//

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
    
    func applicationDidFinishLaunching(_ aNotification: Notification) {
        // Insert code here to initialize your application
    }
    
    func applicationWillTerminate(_ aNotification: Notification) {
        // Insert code here to tear down your application
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
