//
//  ViewController.swift
//  VCPClient
//
//  Created by Björn Eschrich on 25.10.25.
//

import Cocoa

class ViewController: NSViewController {

    override func viewDidLoad() {
        super.viewDidLoad()

        // Do any additional setup after loading the view.
    }
    
    override func viewWillAppear() {
        super.viewWillAppear()
    }
    
    override func willPresentError(_ error: any Error) -> any Error {
        return error
    }

    override func viewWillDisappear() {
        
    }
    
    override func viewDidAppear() {
        UITools.showBasicNotification(body: "Haha")
    }
    
    override var representedObject: Any? {
        didSet {
        // Update the view, if already loaded.
        }
    }
    
    @IBAction
    internal func onInstallDriver(any: Any) {
        NSWorkspace.shared.open(URL(string: "https://github.com/bjoerneschrich/VCPClient")!)
    }

    @IBAction
    internal func onUninstallDriver(any: Any) {
        NSWorkspace.shared.open(URL(string: "https://github.com/bjoerneschrich/VCPClient")!)
    }

}

