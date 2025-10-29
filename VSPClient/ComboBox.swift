// ********************************************************************
// VSPClient User Interface
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
import Cocoa

class ComboBox: NSComboBox, NSComboBoxDelegate, NSTextFieldDelegate, NSComboBoxDataSource {

    struct DataObject: Comparable {
        var id: Int = 0
        var name: String = ""
        var value: Any? = nil
        var description: String {
            return "\(id): \(name)"
        }
        
        init() {}
            
        init(id: Int, name: String, value: Any? = nil) {
            self.id = id
            self.name = name
            self.value = value
        }
        
        // Conformance to Comparable
        static func < (lhs: DataObject, rhs: DataObject) -> Bool {
            return lhs.id < rhs.id
        }
        
        static func == (lhs: DataObject, rhs: DataObject) -> Bool {
            return lhs.id == rhs.id
        }
    }
    
    private var dataObjects: [DataObject] = []
    
    
    override init(frame: NSRect) {
        super.init(frame: frame)
        self.dataSource = self
        self.delegate = self
    }
    
    required init?(coder: NSCoder) {
        super.init(coder: coder)
        self.dataSource = self
        self.delegate = self
    }
    
    override func draw(_ dirtyRect: NSRect) {
        super.draw(dirtyRect)
    }
    
    override func viewDidHide() {
        self.dataSource = nil
        self.delegate = nil
    }
    
    override func viewDidUnhide() {
        self.dataSource = self
        self.delegate = self
    }
    
    override func awakeFromNib() {
        super.awakeFromNib()
        self.dataSource = self
        self.delegate = self
    }

    open func numberOfItems(in comboBox: NSComboBox) -> Int {
        return dataObjects.count
    }

    open func comboBox(_ comboBox: NSComboBox, objectValueForItemAt index: Int) -> Any? {
        if index < 0 || index >= dataObjects.count {
            return nil
        }
        return dataObjects[index].name
    }
    
    open func comboBox(_ comboBox: NSComboBox, indexOfItemWithStringValue string: String) -> Int {
        var index: Int = 0
        for data in dataObjects {
            if data.name == string {
                return index
            }
            index += 1
        }
        return NSNotFound
    }

    open func addItem(withObjectValue text: String) {
        dataObjects.append(DataObject(id: dataObjects.count, name: text, value: nil))
        dataObjects = dataObjects.sorted { $0.id < $1.id }
        super.addItem(withObjectValue: text)
    }

    open func addItem(withText text: String, dataObject data: Any) {
        dataObjects.append(DataObject(id: dataObjects.count, name: text, value: data))
        dataObjects = dataObjects.sorted { $0.id < $1.id }
        super.addItem(withObjectValue: text)
    }
    
    open override func selectItem(withObjectValue id: (Any)?)
    {
        guard id != nil else {
            selectItem(at: 0)
            return
        }
        var index = 0
        dataObjects.forEach { (data) in
            if (data.value as? AnyHashable) == (id as? AnyHashable) {
                super.selectItem(at: index)
                return
            }
            index += 1
        }
    }
    
    open override func itemObjectValue(at index: Int) -> Any
    {
        if index < 0 || index >= dataObjects.count {
            return 0 as Any
        }
        return dataObjects[index].value!
    }
    
    open override var objectValueOfSelectedItem: Any? {
        let index = self.indexOfSelectedItem
        if index < 0 || index >= dataObjects.count {
            return nil
        }
        return dataObjects[index].value
    }

    open func dataObject(forRow index: Int) -> Any? {
        if index < 0 || index >= dataObjects.count {
            return nil
        }
        return dataObjects[index].value
    }

    open func reset() {
        removeAllItems()
    }

    open override func removeAllItems() {
        super.removeAllItems()
        dataObjects.removeAll()
    }
}
