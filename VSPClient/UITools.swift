//
//  UITools.swift
//  VCPClient
//
//  Created by Björn Eschrich on 26.10.25.
//

import Foundation
import Cocoa
import AppKit
import UserNotifications
import SwiftUI

class UITools {
    
    @State static var isNotifyGranted: Bool = false
    
    static public func applicationName() -> String {
        Bundle.main.infoDictionary?["CFBundleName"] as! String
    }
    
    static public func applicationVersion() -> String {
        Bundle.main.infoDictionary?["CFBundleShortVersionString"] as! String
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
    
    // Show a basic notification
    static public func showBasicNotification(title: String = applicationName(),
                                    body: String,
                                    subtitle: String? = nil) {
        if (!UITools.isNotifyGranted) {
            showMessage(message: body)
            return
        }
        let content = UNMutableNotificationContent()
        content.title = title
        content.body = body
        content.subtitle = subtitle ?? ""
        content.sound = UNNotificationSound.default
        
        // Add badge if needed
        content.badge = 1
        
        let request = UNNotificationRequest(
            identifier: UUID().uuidString,
            content: content,
            trigger: nil
        )
        
        UNUserNotificationCenter.current().add(request) { e in
            if let error = e {
                print("Failed to schedule notification: \(error))")
            }
        }
    }
    
    // Show notification with action button
    static public func showNotificationWithAction(title: String = applicationName(),
                                         body: String,
                                         actionTitle: String = "View") {
        if (!UITools.isNotifyGranted) {
            showMessage(message: body)
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
        
        let request = UNNotificationRequest(
            identifier: UUID().uuidString,
            content: content,
            trigger: nil
        )
        
        UNUserNotificationCenter.current().add(request) { e in
            if let error = e {
                print("Failed to schedule notification: \(error))")
            }
        }
    }
    
    // Show notification with custom sound
    static public func showNotificationWithCustomSound(title: String = applicationName(),
                                              body: String,
                                              soundName: String? = nil) {
        if (!UITools.isNotifyGranted) {
            showMessage(message: body)
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
        
        let request = UNNotificationRequest(
            identifier: UUID().uuidString,
            content: content,
            trigger: nil
        )
        
        UNUserNotificationCenter.current().add(request) { e in
            if let error = e {
                print("Failed to schedule notification: \(error))")
            }
        }
    }
    
    // Schedule notification with delay
    static public func scheduleNotification(title: String = applicationName(),
                                   body: String,
                                   delaySeconds: TimeInterval = 5) {
        if (!UITools.isNotifyGranted) {
            showMessage(message: body)
            return
        }
        let content = UNMutableNotificationContent()
        content.title = title
        content.body = body
        content.sound = UNNotificationSound.default
        
        let trigger = UNTimeIntervalNotificationTrigger(timeInterval: delaySeconds, repeats: false)
        
        let request = UNNotificationRequest(
            identifier: UUID().uuidString,
            content: content,
            trigger: trigger
        )
        
        UNUserNotificationCenter.current().add(request) { e in
            if let error = e {
                print("Failed to schedule notification: \(error))")
            }
        }
    }
    
    // Show notification with attachment (image, etc.)
    static public func showNotificationWithAttachment(title: String = applicationName(),
                                             body: String,
                                             attachmentURL: URL) {
        if (!UITools.isNotifyGranted) {
            showMessage(message: body)
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
        
        let request = UNNotificationRequest(
            identifier: UUID().uuidString,
            content: content,
            trigger: nil
        )
        
        UNUserNotificationCenter.current().add(request) { e in
            if let error = e {
                print("Failed to schedule notification: \(error))")
            }
        }
    }
    
    // Show notification with badge
    static func showNotificationWithBadge(title: String = applicationName(),
                                        body: String,
                                        badgeCount: Int) {
        if (!UITools.isNotifyGranted) {
            showMessage(message: body)
            return
        }
        let content = UNMutableNotificationContent()
        content.title = title
        content.body = body
        content.sound = UNNotificationSound.default
        content.badge = NSNumber(value: badgeCount)
        
        let request = UNNotificationRequest(
            identifier: UUID().uuidString,
            content: content,
            trigger: nil
        )
        
        UNUserNotificationCenter.current().add(request) { e in
            if let error = e {
                print("Failed to schedule notification: \(error)")
            }
        }
    }
}
