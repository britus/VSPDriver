//
//  WindowController.swift
//  VCPClient
//
//  Created by Björn Eschrich on 26.10.25.
//

import Cocoa
import SwiftUI

class WindowController: NSWindowController {
    @IBOutlet weak var tbSerialPorts: NSToolbarItem!
    @IBOutlet weak var tbSerialTest: NSToolbarItem!
    @IBOutlet weak var tbPortLinks: NSToolbarItem!
    @IBOutlet weak var tbMessages: NSToolbarItem!
    @IBOutlet weak var toolBar: NSToolbar!
    private weak var tabViewController: TabViewController!

    @State private var isNotifyGranted: Bool = false

    override func windowWillLoad() {
        NSLog("windowWillLoad")
    }
    
    override func windowDidLoad() {
        NSLog("windowDidLoad")
        NSLog("\(String(describing: self.contentViewController))")
        tabViewController = ((self.contentViewController as! NSTabViewController) as! TabViewController)
        
        // Request permission first (required)
        UITools.requestNotificationPermission { granted in
            if granted {
                //print("Notification permission granted")
                self.isNotifyGranted = true
            } else {
                //print("Notification permission denied")
                self.isNotifyGranted = false
            }
        }
     }
    
    override func windowTitle(forDocumentDisplayName displayName: String) -> String {
        return "VSP Controller"
    }
    
    @IBAction func onSerialPorts(_ sender: Any) {
        tabViewController.selectedTabViewItemIndex = 0
    }
    
    @IBAction func onPortLinks(_ sender: Any) {
        tabViewController.selectedTabViewItemIndex = 1
    }
    
    @IBAction func onSerialTest(_ sender: Any) {
        
    }
}
