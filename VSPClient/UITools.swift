// ********************************************************************
// VSPClient User Interface
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
import Foundation
import Cocoa
import AppKit
import UserNotifications
import SwiftUI

class UITools {
    
    public static var isNotifyGranted: Bool = false

    static public func applicationName() -> String {
        Bundle.main.infoDictionary?["CFBundleDisplayName"] as! String
    }
    
    static public func applicationVersion() -> String {
        Bundle.main.infoDictionary?["CFBundleShortVersionString"] as! String
    }

    static public func applicationBuild() -> String {
        Bundle.main.infoDictionary?["CFBundleVersion"] as! String
    }

    // Basic alert with title and message
    static public func showMessage(message: String, info: String? = nil) {
        DispatchQueue.main.async {
            let alert = NSAlert()
            alert.window.title = applicationName()
            alert.messageText = message
            alert.informativeText = ((info?.isEmpty) != nil) ? info! : ""
            alert.alertStyle = .informational
            alert.runModal()
        }
    }
    
    static func showQuestionDialog(_ sender: Any, _ message: String, info: String? = nil) -> Bool {
            // Create the alert
            let alert = NSAlert()
        
            // Configure the alert
            alert.window.title = applicationName()
            alert.messageText = message
            alert.informativeText = ((info?.isEmpty) != nil) ? info! : ""
            alert.alertStyle = .informational
            
            // Add buttons
            alert.addButton(withTitle: "Yes")
            alert.addButton(withTitle: "No")
            
            // Show the alert and get the response
            let response = alert.runModal()
            
            // Handle the response
            switch response {
                case .alertFirstButtonReturn: // "Yes" button
                    return true
                case .alertSecondButtonReturn: // "No" button
                    return false
                default:
                    return false
            }
    }
    
    // Request notification permissions (required for macOS 10.14+)
    static public func requestNotificationPermission(completion: @escaping (Bool) -> Void) {
        UNUserNotificationCenter.current().requestAuthorization(options: [.alert, .sound, .badge]) { granted, e in
            DispatchQueue.main.async {
                if let error = e {
                    print("Notification permission error: \(error.localizedDescription))")
                    UITools.isNotifyGranted = false
                    completion(false)
                } else {
                    UITools.isNotifyGranted = granted
                    completion(granted)
                }
            }
        }
    }
    
    static private func scheduleNotification(id: String = UUID().uuidString, content: UNMutableNotificationContent)
    {
        //let trigger = UNTimeIntervalNotificationTrigger(timeInterval: 2, repeats: false)
        let mr = UNNotificationRequest(
            identifier: "mr_\(id)",
            content: content,
            trigger: nil
        )
        
        UNUserNotificationCenter.current().add(mr) { e in
            if let error = e {
                print("Failed to schedule main notification: \(error)")
            }
        }
    }
    
    // Show a basic notification
    static public func showBasicNotification(title: String = applicationName(),
                                    body: String,
                                    subtitle: String? = nil) {
        if (!UITools.isNotifyGranted) {
            return
        }
        let content = UNMutableNotificationContent()
        content.title = title
        content.body = body
        content.subtitle = subtitle ?? ""
        content.sound = UNNotificationSound.default
        
        // Add badge if needed
        content.badge = 1
        
        scheduleNotification(content: content)
    }
    
    // Show notification with action button
    static public func showNotificationWithAction(title: String = applicationName(),
                                         body: String,
                                         actionTitle: String = "View") {
        if (!UITools.isNotifyGranted) {
             return
        }
        
        let content = UNMutableNotificationContent()
        content.title = title
        content.body = body
        content.sound = UNNotificationSound.default
        
        // Create action
        let viewAction = UNNotificationAction(
            identifier: "view_action",
            title: actionTitle,
            options: [.foreground]
        )
        
        // Create category
        let category = UNNotificationCategory(
            identifier: "notification_category",
            actions: [viewAction],
            intentIdentifiers: [],
            options: []
        )
        
        UNUserNotificationCenter.current().setNotificationCategories([category])
        content.categoryIdentifier = "notification_category"
        
        scheduleNotification(content: content)
    }
    
    // Show notification with custom sound
    static public func showNotificationWithCustomSound(title: String = applicationName(),
                                              body: String,
                                              soundName: String? = nil) {
        if (!UITools.isNotifyGranted) {
            return
        }
        let content = UNMutableNotificationContent()
        content.title = title
        content.body = body
        
        if let soundName = soundName {
            content.sound = UNNotificationSound(named: UNNotificationSoundName(soundName))
        } else {
            content.sound = UNNotificationSound.default
        }

        scheduleNotification(content: content)
    }
    
    // Schedule notification with delay
    static public func scheduleNotification(title: String = applicationName(),
                                   body: String,
                                   delaySeconds: TimeInterval = 5) {
        if (!UITools.isNotifyGranted) {
            return
        }
        let content = UNMutableNotificationContent()
        content.title = title
        content.body = body
        content.sound = UNNotificationSound.default
        
        scheduleNotification(content: content)
    }
    
    // Show notification with attachment (image, etc.)
    static public func showNotificationWithAttachment(title: String = applicationName(),
                                             body: String,
                                             attachmentURL: URL) {
        if (!UITools.isNotifyGranted) {
            return
        }
        let content = UNMutableNotificationContent()
        content.title = title
        content.body = body
        content.sound = UNNotificationSound.default
        
        do {
            let attachment = try UNNotificationAttachment(
                identifier: "image",
                url: attachmentURL,
                options: nil
            )
            content.attachments = [attachment]
        } catch {
            print("Failed to create attachment: $error)")
        }
        
        scheduleNotification(content: content)
    }
    
    // Show notification with badge
    static func showNotificationWithBadge(title: String = applicationName(),
                                        body: String,
                                        badgeCount: Int) {
        if (!UITools.isNotifyGranted) {
            return
        }
        let content = UNMutableNotificationContent()
        content.title = title
        content.body = body
        content.sound = UNNotificationSound.default
        content.badge = NSNumber(value: badgeCount)

        scheduleNotification(content: content)
    }
    
    // Method to populate Baud Rate combo box
    static func populateBaudRateComboBox(_ comboBox: ComboBox) {
        let values: [UInt32] = [
            50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800, 2400, 4800,
            7200, 9600, 14400, 19200, 28800, 38400, 57600, 115200
            ,128000,
            230400, 256000, 460800, 512000, 921600, 1024000, 1843200, 2048000,
            3686400, 4096000, 7372800, 14745600, 29491200, 38400000, 76800000
        ]
        
        comboBox.removeAllItems()
        for value in values {
            comboBox.addItem(withText: String(value), dataObject: value)
        }
        if comboBox.numberOfItems > 0 {
            comboBox.selectItem(withObjectValue: UInt32(115200))
        }
    }

    // Method to populate Data Bits combo box
    static func populateDataBitsComboBox(_ comboBox: ComboBox) {
        comboBox.removeAllItems()
        comboBox.addItem(withText: "5 Bits", dataObject: UInt8(5))
        comboBox.addItem(withText: "6 Bits", dataObject: UInt8(6))
        comboBox.addItem(withText: "7 Bits", dataObject: UInt8(7))
        comboBox.addItem(withText: "8 Bits", dataObject: UInt8(8))
        comboBox.selectItem(at: comboBox.numberOfItems-1)
    }

    // Method to populate Stop Bits combo box
    static func populateStopBitsComboBox(_ comboBox: ComboBox) {
        comboBox.removeAllItems()
        comboBox.addItem(withText: "One Stop", dataObject: UInt8(1))
        comboBox.addItem(withText: "Two Stop", dataObject: UInt8(2))
        comboBox.selectItem(at: 0)
    }

    // Method to populate Parity combo box
    static func populateParityComboBox(_ comboBox: ComboBox) {
        comboBox.removeAllItems()
        comboBox.addItem(withText: "None", dataObject: UInt8(0))
        comboBox.addItem(withText: "Odd", dataObject: UInt8(1))
        comboBox.addItem(withText: "Even", dataObject: UInt8(2))
        comboBox.selectItem(at: 0)
    }

    // Method to populate Flow Control combo box
    static func populateFlowControlComboBox(_ comboBox: ComboBox) {
        comboBox.removeAllItems()
        comboBox.addItem(withText: "None", dataObject: UInt8(0))
        comboBox.addItem(withText: "Hardware Control", dataObject: UInt8(1))
        comboBox.addItem(withText: "Xon/Xoff Control", dataObject: UInt8(2))
        comboBox.selectItem(at: 0)
    }

    // Helper method to get selected value from combo box
    static func selectedValueFor(_ comboBox: ComboBox) -> Any? {
        let index = comboBox.indexOfSelectedItem
        comboBox.selectItem(at: index)
        guard let selectedItem = comboBox.dataObject(forRow: index) else {
            return nil
        }
        return selectedItem
    }
}

extension String {
    /// Checks if the string starts with the specified substring
    /// - Parameter with: The substring to check for at the beginning
    /// - Returns: true if the string starts with the specified substring, false otherwise
    func startsWith(_ value: String) -> Bool {
        return self.hasPrefix(value)
    }
    
    /// Checks if the string ends with the specified substring
    /// - Parameter with: The substring to check for at the end
    /// - Returns: true if the string ends with the specified substring, false otherwise
    func endsWith(_ value: String) -> Bool {
        return self.hasSuffix(value)
    }
}

extension NSMutableAttributedString {
    /// Checks if the string starts with the specified substring
    /// - Parameter with: The substring to check for at the beginning
    /// - Returns: true if the string starts with the specified substring, false otherwise
    func startsWith(_ value: String) -> Bool {
        return self.string.hasPrefix(value)
    }
    
    /// Checks if the string ends with the specified substring
    /// - Parameter with: The substring to check for at the end
    /// - Returns: true if the string ends with the specified substring, false otherwise
    func endsWith(_ value: String) -> Bool {
        return self.string.hasSuffix(value)
    }
    
    func append(_ value: String) {
        self.append(NSMutableAttributedString(string: value))
    }
}

extension NSTextView {
    func scrollToEndOfTextView() {
        guard let textContainer = self.textContainer else { return }
        
        // Get the text container's size
        let contentSize = textContainer.size
        
        // Scroll to the bottom
        self.scroll(NSPoint(x: 0, y: contentSize.height))
    }

    func scrollToBottomOfTextView() {
        guard let textContainer = self.textContainer else { return }
        
        // Get the content size
        let contentSize = textContainer.size
        
        // Calculate the scroll position (bottom)
        let scrollPoint = NSPoint(x: 0, y: contentSize.height)
        
        // Scroll to the calculated point
        self.scroll(scrollPoint)
    }

    // Method 5: Complete solution with proper handling
    func scrollToTextEnd() {
        guard let textStorage = self.textStorage else { return }
         
         // Get the range of the last character
         let length = textStorage.length
         if length > 0 {
             let range = NSRange(location: length - 1, length: 1)
             self.scrollRangeToVisible(range)
         } else {
             // If empty text, scroll to beginning
             self.scrollRangeToVisible(NSRange(location: 0, length: 0))
         }
    }
    
    func scrollToTextViewEnd() {
        guard let layoutManager = self.layoutManager,
              let textContainer = self.textContainer else { return }
        let _ : Bool = layoutManager.isProxy() /* fix swift bullshit warning */
        
        // Alternative approach using the text container's actual size
        let contentSize = textContainer.size
        
        // Get the visible rectangle to calculate proper scroll position
        let visibleRect = self.visibleRect
        
        // Calculate the bottom position (this is more accurate)
        let scrollY = max(0, contentSize.height - visibleRect.height)
        
        // Scroll to the calculated point
        self.scroll(NSPoint(x: 0, y: scrollY))
        
        // Additional safety - ensure we're at the very end
        if let textStorage = self.textStorage {
            let endRange = NSRange(location: textStorage.length, length: 0)
            self.scrollRangeToVisible(endRange)
        }
    }
    
    /// Format text with specific background color, foreground color, and font
    /// - Parameters:
    ///   - text: The text to format
    ///   - backgroundColor: Background color for the text
    ///   - textColor: Foreground color (text color)
    ///   - fontName: Font name (e.g., "Menlo", "Monaco")
    ///   - fontSize: Font size
    func setText(_ text: String,
                   backgroundColor: NSColor = .clear,
                   textColor: NSColor = .white,
                   fontName: String = "Menlo",
                   fontSize: CGFloat = 15.0) {
        
        // Create attributes dictionary
        let attributes: [NSAttributedString.Key: Any] = [
            .backgroundColor: backgroundColor,
            .foregroundColor: textColor,
            .font: NSFont.monospacedSystemFont(ofSize: fontSize, weight: .regular)
        ]
        
        // Create attributed string
        let attributedString = NSAttributedString(string: text, attributes: attributes)
        
        // Set the attributed string to the text view
        self.textStorage?.setAttributedString(attributedString)
    }
}

