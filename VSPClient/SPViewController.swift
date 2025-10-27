//
//  SPViewController.swift
//  VCPClient
//
//  Created by Björn Eschrich on 26.10.25.
//

import Cocoa

class SPViewController: NSViewController {
    
    @IBOutlet weak var pnlSerialPort: NSView!
    @IBOutlet weak var cbxBaudRate: NSComboBox!
    @IBOutlet weak var cbxDataBits: NSComboBox!
    @IBOutlet weak var cbxStopBits: NSComboBox!
    @IBOutlet weak var cbxParity: NSComboBox!
    @IBOutlet weak var cbxFlowCtrl: NSComboBox!
    @IBOutlet weak var tbvPortList: TableView!
    @IBOutlet weak var pbCreatePort: NSButton!
    var model: DriverModel = DriverModel()
    
    private var records: [TDataRecord] = []
    
    override func viewDidLoad() {
        super.viewDidLoad()
        // Do view setup here.
    }
    
    @IBAction func onBaudRateChanged(_ sender: NSComboBox) {
    }
    
    @IBAction func onDataBitsChanged(_ sender: NSComboBox) {
    }
    
    @IBAction func onStopBitsChanged(_ sender: NSComboBox) {
    }
    
    @IBAction func onParityChanged(_ sender: NSComboBox) {
    }

    @IBAction func onFlowCtrlChanged(_ sender: NSComboBox) {
    }
    
    @IBAction func onCreatePort(_ sender: NSButton) {
    }
}
