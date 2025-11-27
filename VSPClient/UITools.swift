import AppKit
import Cocoa
// ********************************************************************
// VSPClient User Interface
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
import Foundation
import SwiftUI
import UserNotifications

private var progressIndicatorKey: UInt8 = 0xf1
private var progressOverlayKey: UInt8 = 0xf2
private var viewControllerKey: UInt8 = 0xf3

// MARK: Extension to add queue checking to DispatchQueue

extension DispatchQueue {
    static func getCurrentQueueName() -> String {
        return Thread.current.name ?? "Unknown"
    }

    static func isMainQueue() -> Bool {
        let name = DispatchQueue.getCurrentQueueName()
        let main = DispatchQueue.main.label
        return main == name
    }

    static func isOnMainQueue() -> Bool {
        return Thread.isMainThread
    }
}

// MARK: Custom alert dialog

class AlertDialog: NSViewController {

    private let titleLabel = NSTextField(
        labelWithString: ""
    )
    private let subtitleLabel = NSTextField(
        labelWithString: ""
    )

    private let textView = NSTextView()
    private let scrollView = NSScrollView()

    private let separator = NSBox()
    private let okButton = NSButton()

    var titleText: String = ""
    var subtitleText: String = ""  // Optional, can be empty
    var messageText: String = ""
    var completion:
        (
            () -> Void
        )?

    init(
        title: String,
        subtitle: String = "",
        message: String,
        completion: (
            () -> Void
        )? = nil
    ) {
        super.init(
            nibName: nil,
            bundle: nil
        )
        self.titleText = title
        self.subtitleText = subtitle
        self.messageText = message
        self.completion = completion
    }

    required init?(
        coder: NSCoder
    ) {
        fatalError(
            "init(coder:) has not been implemented"
        )
    }

    override func loadView() {
        self.view = NSView()
        self.view.translatesAutoresizingMaskIntoConstraints = false
        self.preferredContentSize = NSSize(
            width: 420,
            height: 260
        )
    }

    override func viewDidLoad() {
        super.viewDidLoad()
        configureWindow()
        buildUILayout()
        applyConstraints()
    }

    override func viewWillAppear() {
        super.viewWillAppear()
        self.view.window?.title = titleText
    }

    private func configureWindow() {
        if let w = self.view.window {
            w.styleMask
                .insert(
                    .resizable
                )
        }
    }

    private func buildUILayout() {

        // Title
        titleLabel.stringValue = titleText
        titleLabel.font =
            NSFont
            .boldSystemFont(
                ofSize: 16
            )
        titleLabel.alignment = .left

        // Subtitle
        subtitleLabel.stringValue = subtitleText
        subtitleLabel.font =
            NSFont
            .systemFont(
                ofSize: 12
            )
        subtitleLabel.textColor = .secondaryLabelColor
        subtitleLabel.alignment = .left

        // Text View inside Scroll View
        textView.isEditable = false
        textView.isSelectable = true
        textView.string = messageText
        textView.font =
            NSFont
            .systemFont(
                ofSize: 13
            )
        textView.textContainerInset = NSSize(
            width: 8,
            height: 8
        )
        textView.isVerticallyResizable = true
        textView.isHorizontallyResizable = false
        textView.autoresizingMask = [.width]

        // IMPORTANT: Wrap text properly
        if let container = textView.textContainer {
            container.widthTracksTextView = true
            container.heightTracksTextView = false
            container.lineFragmentPadding = 4
        }

        scrollView.documentView = textView
        scrollView.hasVerticalScroller = true
        scrollView.borderType = .bezelBorder
        scrollView.translatesAutoresizingMaskIntoConstraints = false

        // Separator
        separator.boxType = .separator

        // OK Button
        okButton.title = "OK"
        okButton.target = self
        okButton.action = #selector(
            okButtonClicked
        )

        // Add to view
        [
            titleLabel,
            subtitleLabel,
            scrollView,
            separator,
            okButton,
        ].forEach {
            $0.translatesAutoresizingMaskIntoConstraints = false
            view
                .addSubview(
                    $0
                )
        }
    }

    private func applyConstraints() {

        NSLayoutConstraint
            .activate(
                [
                    // Title
                    titleLabel.topAnchor
                        .constraint(
                            equalTo: view.topAnchor,
                            constant: 16
                        ),
                    titleLabel.leadingAnchor
                        .constraint(
                            equalTo: view.leadingAnchor,
                            constant: 16
                        ),
                    titleLabel.trailingAnchor
                        .constraint(
                            equalTo: view.trailingAnchor,
                            constant: -16
                        ),

                    // Subtitle
                    subtitleLabel.topAnchor
                        .constraint(
                            equalTo: titleLabel.bottomAnchor,
                            constant: 4
                        ),
                    subtitleLabel.leadingAnchor
                        .constraint(
                            equalTo: titleLabel.leadingAnchor
                        ),
                    subtitleLabel.trailingAnchor
                        .constraint(
                            equalTo: titleLabel.trailingAnchor
                        ),

                    // Scroll View
                    scrollView.topAnchor
                        .constraint(
                            equalTo: subtitleLabel.bottomAnchor,
                            constant: 12
                        ),
                    scrollView.leadingAnchor
                        .constraint(
                            equalTo: titleLabel.leadingAnchor
                        ),
                    scrollView.trailingAnchor
                        .constraint(
                            equalTo: titleLabel.trailingAnchor
                        ),
                    scrollView.bottomAnchor
                        .constraint(
                            equalTo: separator.topAnchor,
                            constant: -12
                        ),

                    // Separator
                    separator.leadingAnchor
                        .constraint(
                            equalTo: view.leadingAnchor
                        ),
                    separator.trailingAnchor
                        .constraint(
                            equalTo: view.trailingAnchor
                        ),
                    separator.heightAnchor
                        .constraint(
                            equalToConstant: 1
                        ),
                    separator.bottomAnchor
                        .constraint(
                            equalTo: okButton.topAnchor,
                            constant: -12
                        ),

                    // OK Button bottom-right
                    okButton.trailingAnchor
                        .constraint(
                            equalTo: view.trailingAnchor,
                            constant: -16
                        ),
                    okButton.bottomAnchor
                        .constraint(
                            equalTo: view.bottomAnchor,
                            constant: -12
                        ),
                ]
            )
    }

    @objc private func okButtonClicked() {
        AppDelegate.window?.contentViewController?
            .dismiss(
                self
            )
        DispatchQueue.main
            .async {
                self.completion?()
            }
    }
}

// MARK: Tools

class UITools {

    public static var isNotifyGranted: Bool = false

    static public
        func applicationName() -> String
    {
        return Bundle.main.infoDictionary?["CFBundleDisplayName"] as! String
    }

    static public
        func applicationVersion() -> String
    {
        return Bundle.main.infoDictionary?["CFBundleShortVersionString"]
            as! String
    }

    static public
        func applicationBuild() -> String
    {
        return Bundle.main.infoDictionary?["CFBundleVersion"] as! String
    }

    static public
        func bundleId() -> String
    {
        return Bundle.main.bundleIdentifier!
    }

    static public
        func bundleResourceUrl() -> URL
    {
        return Bundle.main.resourceURL!
    }

    static public
        func bundleResourcePath() -> String
    {
        return Bundle.main.resourcePath!
    }

    static private
        func presentAlert(
            _ vcAlert: AlertDialog
        )
    {
        guard let window = AppDelegate.window else {
            NSLog(
                "UITools: No main window!"
            )
            return
        }
        guard let cvc = window.contentViewController else {
            NSLog(
                "UITools: No content view controller!"
            )
            window.contentViewController = vcAlert
            return
        }
        //cvc.presentAsModalWindow(vcAlert)
        cvc
            .presentAsSheet(
                vcAlert
            )
    }

    static public
        func showAlert(
            title: String,
            message: String,
            completion: (
                () -> Void
            )? = nil
        )
    {
        if DispatchQueue
            .isOnMainQueue()
        {
            let vcAlert = AlertDialog(
                //
                title: title,
                message: message,
                completion: completion
            )
            presentAlert(
                vcAlert
            )
        } else {
            DispatchQueue.main
                .async {
                    let vcAlert = AlertDialog(
                        //
                        title: title,
                        message: message,
                        completion: completion
                    )
                    presentAlert(
                        vcAlert
                    )
                }
        }
    }

    // Show alert dialog async
    static public
        func showMessage(
            message: String,
            info: String? = nil,
            withCompletion completion: (
                () -> Void
            )? = nil
        )
    {
        DispatchQueue.main
            .async {
                var text: String = message
                if let infoText = info {
                    text += "\n\(infoText)"
                }
                showAlert(
                    title: "Notice",
                    message: text,
                    completion: completion
                )
            }
    }

    // Show alert dialog async
    static public
        func showMessage(
            message: String,
            info: String? = nil
        )
    {
        showMessage(
            message: message,
            info: info,
            withCompletion: nil
        )
    }

    static public
        func showQuestionDialog(
            _ sender: Any,
            _ message: String,
            info: String? = nil
        ) -> Bool
    {
        // Create the alert
        let alert = NSAlert()

        // Configure the alert
        alert.window.title = applicationName()
        alert.messageText = message
        alert.informativeText = ((info?.isEmpty) != nil) ? info! : ""
        alert.alertStyle = .informational

        // Add buttons
        alert
            .addButton(
                withTitle: "Yes"
            )
        alert
            .addButton(
                withTitle: "No"
            )

        // Show the alert and get the response
        let response = alert.runModal()

        // Handle the response
        switch response {
        case .alertFirstButtonReturn:  // "Yes" button
            return true
        case .alertSecondButtonReturn:  // "No" button
            return false
        default:
            return false
        }
    }

    // Request notification permissions (required for macOS 10.14+)
    static public
        func requestNotificationPermission(
            completion:
                @escaping (
                    Bool
                ) -> Void
        )
    {
        UNUserNotificationCenter
            .current()
            .requestAuthorization(
                options: [
                    .alert,
                    .sound,
                    .badge,
                ]
            ) {
                granted,
                e in
                DispatchQueue.main
                    .async {
                        if let error = e {
                            print(
                                "Notification permission error: \(error.localizedDescription))"
                            )
                            UITools.isNotifyGranted = false
                            completion(
                                false
                            )
                        } else {
                            UITools.isNotifyGranted = granted
                            completion(
                                granted
                            )
                        }
                    }
            }
    }

    static private
        func scheduleNotification(
            content: UNMutableNotificationContent
        )
    {
        if !UITools.isNotifyGranted {
            return
        }
        DispatchQueue.main
            .async {
                //let trigger = UNTimeIntervalNotificationTrigger(timeInterval: 2, repeats: false)
                let mr = UNNotificationRequest(
                    identifier: UUID().uuidString,
                    content: content,
                    trigger: nil
                )
                UNUserNotificationCenter
                    .current()
                    .removeAllDeliveredNotifications()
                UNUserNotificationCenter
                    .current()
                    .removeAllPendingNotificationRequests()
                UNUserNotificationCenter
                    .current()
                    .add(
                        mr
                    ) { e in
                        if let error = e {
                            print(
                                "Failed to schedule main notification: \(error)"
                            )
                        }
                    }
            }
    }

    // Show a basic notification
    static public
        func showNotification(
            title: String = applicationName(),
            body: String,
            subtitle: String? = nil
        )
    {
        let content = UNMutableNotificationContent()
        content.title = title
        content.body = body
        content.subtitle = subtitle ?? ""
        content.sound = UNNotificationSound.default
        content.badge = 0  // Add badge if needed
        scheduleNotification(
            content: content
        )
    }

    // Show notification with action button
    static public
        func showNotificationWithAction(
            title: String = applicationName(),
            body: String,
            actionTitle: String = "View"
        )
    {
        let content = UNMutableNotificationContent()
        content.title = title
        content.body = body
        content.sound = UNNotificationSound.default
        content.badge = 0  // Add badge if needed

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

        UNUserNotificationCenter
            .current()
            .setNotificationCategories(
                [category]
            )
        content.categoryIdentifier = "notification_category"

        scheduleNotification(
            content: content
        )
    }

    // Show notification with custom sound
    static public
        func showNotificationWithCustomSound(
            title: String = applicationName(),
            body: String,
            soundName: String? = nil
        )
    {
        let content = UNMutableNotificationContent()
        content.title = title
        content.body = body

        if let soundName = soundName {
            content.sound = UNNotificationSound(
                named: UNNotificationSoundName(
                    soundName
                )
            )
        } else {
            content.sound = UNNotificationSound.default
        }

        scheduleNotification(
            content: content
        )
    }

    // Show notification with attachment (image, etc.)
    static public
        func showNotificationWithAttachment(
            title: String = applicationName(),
            body: String,
            attachmentURL: URL
        )
    {
        let content = UNMutableNotificationContent()
        content.title = title
        content.body = body
        content.sound = UNNotificationSound.default
        content.badge = 0  // Add badge if needed

        do {
            let attachment = try UNNotificationAttachment(
                identifier: "image",
                url: attachmentURL,
                options: nil
            )
            content.attachments = [attachment]
        } catch {
            NSLog(
                "UITools: Failed to create attachment: \(error)"
            )
        }

        scheduleNotification(
            content: content
        )
    }

    // Show notification with badge
    static public
        func showNotificationWithBadge(
            title: String = applicationName(),
            subTitle: String = "Notice",
            body: String,
            badgeCount: Int
        )
    {
        let content = UNMutableNotificationContent()
        content.title = title
        content.subtitle = subTitle
        content.body = body
        content.sound = UNNotificationSound.default
        content.badge = NSNumber(
            value: badgeCount
        )
        scheduleNotification(
            content: content
        )
    }

    // Method to populate Baud Rate combo box
    static public
        func populateBaudRateComboBox(
            _ comboBox: ComboBox
        )
    {
        let values: [UInt32] = [
            50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800, 2400, 4800,
            7200, 9600, 14400, 19200, 28800, 38400, 57600, 115200, 128000,
            230400, 256000, 460800, 512000, 921600, 1024000, 1843200,
            2048000, 3686400, 4096000, 7372800, 14745600, 29491200,
            38400000, 76800000,
        ]

        comboBox
            .removeAllItems()
        for value in values {
            comboBox
                .addItem(
                    withText: String(
                        value
                    ),
                    dataObject: value
                )
        }
        if comboBox.numberOfItems > 0 {
            comboBox
                .selectItem(
                    withObjectValue: UInt32(
                        115200
                    )
                )
        }
    }

    // Method to populate Data Bits combo box
    static public
        func populateDataBitsComboBox(
            _ comboBox: ComboBox
        )
    {
        comboBox
            .removeAllItems()
        comboBox
            .addItem(
                withText: "5 Bits",
                dataObject: UInt8(
                    5
                )
            )
        comboBox
            .addItem(
                withText: "6 Bits",
                dataObject: UInt8(
                    6
                )
            )
        comboBox
            .addItem(
                withText: "7 Bits",
                dataObject: UInt8(
                    7
                )
            )
        comboBox
            .addItem(
                withText: "8 Bits",
                dataObject: UInt8(
                    8
                )
            )
        comboBox
            .selectItem(
                at: comboBox.numberOfItems - 1
            )
    }

    // Method to populate Stop Bits combo box
    static public
        func populateStopBitsComboBox(
            _ comboBox: ComboBox
        )
    {
        comboBox
            .removeAllItems()
        comboBox
            .addItem(
                withText: "One Stop",
                dataObject: UInt8(
                    1
                )
            )
        comboBox
            .addItem(
                withText: "Two Stop",
                dataObject: UInt8(
                    2
                )
            )
        comboBox
            .selectItem(
                at: 0
            )
    }

    // Method to populate Parity combo box
    static public
        func populateParityComboBox(
            _ comboBox: ComboBox
        )
    {
        comboBox
            .removeAllItems()
        comboBox
            .addItem(
                withText: "None",
                dataObject: UInt8(
                    0
                )
            )
        comboBox
            .addItem(
                withText: "Odd",
                dataObject: UInt8(
                    1
                )
            )
        comboBox
            .addItem(
                withText: "Even",
                dataObject: UInt8(
                    2
                )
            )
        comboBox
            .selectItem(
                at: 0
            )
    }

    // Method to populate Flow Control combo box
    static public
        func populateFlowControlComboBox(
            _ comboBox: ComboBox
        )
    {
        comboBox
            .removeAllItems()
        comboBox
            .addItem(
                withText: "None",
                dataObject: UInt8(
                    0
                )
            )
        comboBox
            .addItem(
                withText: "Hardware Control",
                dataObject: UInt8(
                    1
                )
            )
        comboBox
            .addItem(
                withText: "Xon/Xoff Control",
                dataObject: UInt8(
                    2
                )
            )
        comboBox
            .selectItem(
                at: 0
            )
    }

    // Helper method to get selected value from combo box
    static public
        func selectedValueFrom(
            _ comboBox: ComboBox
        ) -> Any?
    {
        if let item = comboBox.dataObject(
            forRow: comboBox.indexOfSelectedItem
        ) {
            return item
        }
        return nil
    }
}

// MARK: - String Extension

extension String {
    // Checks if the string starts with the specified substring
    // - Parameter with: The substring to check for at the beginning
    // - Returns: true if the string starts with the specified substring, false otherwise
    public func startsWith(
        _ value: String
    ) -> Bool {
        return self.hasPrefix(
            value
        )
    }

    // Checks if the string ends with the specified substring
    // - Parameter with: The substring to check for at the end
    // - Returns: true if the string ends with the specified substring, false otherwise
    public func endsWith(
        _ value: String
    ) -> Bool {
        return self.hasSuffix(
            value
        )
    }
}

extension NSMutableAttributedString {
    // Checks if the string starts with the specified substring
    // - Parameter with: The substring to check for at the beginning
    // - Returns: true if the string starts with the specified substring, false otherwise
    public func startsWith(
        _ value: String
    ) -> Bool {
        return self.string
            .hasPrefix(
                value
            )
    }

    // Checks if the string ends with the specified substring
    // - Parameter with: The substring to check for at the end
    // - Returns: true if the string ends with the specified substring, false otherwise
    public func endsWith(
        _ value: String
    ) -> Bool {
        return self.string
            .hasSuffix(
                value
            )
    }

    public func append(
        _ value: String
    ) {
        self.append(
            NSMutableAttributedString(
                string: value
            )
        )
    }
}

extension StringProtocol {
    subscript(
        _ offset: Int
    ) -> Element {
        self[
            index(
                startIndex,
                offsetBy: offset
            )
        ]
    }
    subscript(
        _ range: Range<Int>
    ) -> SubSequence {
        prefix(
            range.lowerBound + range.count
        ).suffix(
            range.count
        )
    }
    subscript(
        _ range: ClosedRange<Int>
    ) -> SubSequence {
        prefix(
            range.lowerBound + range.count
        ).suffix(
            range.count
        )
    }
    subscript(
        _ range: PartialRangeThrough<Int>
    ) -> SubSequence {
        prefix(
            range.upperBound.advanced(
                by: 1
            )
        )
    }
    subscript(
        _ range: PartialRangeUpTo<Int>
    ) -> SubSequence {
        prefix(
            range.upperBound
        )
    }
    subscript(
        _ range: PartialRangeFrom<Int>
    ) -> SubSequence {
        suffix(
            Swift.max(
                0,
                count - range.lowerBound
            )
        )
    }
}

extension LosslessStringConvertible {
    var string: String {
        .init(
            self
        )
    }
}

extension BidirectionalCollection {
    subscript(
        safe offset: Int
    ) -> Element? {
        guard !isEmpty,
            let i = index(
                startIndex,
                offsetBy: offset,
                limitedBy: index(
                    before: endIndex
                )
            )
        else {
            return nil
        }
        return self[i]
    }
}

// MARK: - String numeric conversions

extension String {
    // Trimmed copy used by all conversions.
    private var trimmed: String {
        return self.trimmingCharacters(
            in: CharacterSet.whitespacesAndNewlines
        )
    }

    // Converts the string to an `Int` if possible.
    var intValue: Int? {
        return Int(
            trimmed
        )
    }

    // Converts the string to a `Double` if possible.
    var doubleValue: Double? {
        return Double(
            trimmed
        )
    }

    // Converts the string to a `Float` if possible.
    var floatValue: Float? {
        return Float(
            trimmed
        )
    }

    // Converts the string to an `Int64` if possible.
    var int64Value: Int64? {
        return Int64(
            trimmed
        )
    }

    // Converts the string to an `UInt32` if possible.
    var uint32Value: UInt32? {
        return UInt32(
            trimmed
        )
    }

    // Converts the string to an `UInt32` if possible.
    var uint64Value: UInt64? {
        return UInt64(
            trimmed
        )
    }

    // Converts the string to a `Decimal` if possible.
    var decimalValue: Decimal? {
        return Decimal(
            string: trimmed
        )
    }

    // Detects and converts the string to an `NSNumber` (Int or Double) using a NumberFormatter.
    // Returns nil if the string cannot be parsed as a number.
    var numericValue: NSNumber? {
        let formatter = NumberFormatter()
        // use a stable locale to avoid surprises (decimal separators).
        formatter.locale = Locale(
            identifier: "en_US_POSIX"
        )
        // allow lenient parsing
        formatter.isLenient = true
        return
            formatter
            .number(
                from: trimmed
            )
    }
}

// MARK: - NSAttributedString convenience

extension NSAttributedString {
    // Numeric parsing forwarded from the attributed string's `string` property.
    var intValue: Int? {
        return self.string.intValue
    }
    var doubleValue: Double? {
        return self.string.doubleValue
    }
    var floatValue: Float? {
        return self.string.floatValue
    }
    var int64Value: Int64? {
        return self.string.int64Value
    }
    var decimalValue: Decimal? {
        return self.string.decimalValue
    }
    var numericValue: NSNumber? {
        return self.string.numericValue
    }
}

// Mutable subclass inherits NSAttributedString implementation
extension NSMutableAttributedString {
    // Inherits the NSAttributedString computed properties;
    // these extensions exist so they're discoverable on
    // NSMutableAttributedString too.
}

// MARK: - NSTextView Extension

extension NSTextView {

    public func scrollToEndOfTextView() {
        guard let textContainer = self.textContainer else {
            return
        }

        // Get the text container's size
        let contentSize = textContainer.size

        // Scroll to the bottom
        self.scroll(
            NSPoint(
                x: 0,
                y: contentSize.height
            )
        )
    }

    public func scrollToBottomOfTextView() {
        guard let textContainer = self.textContainer else {
            return
        }

        // Get the content size
        let contentSize = textContainer.size

        // Calculate the scroll position (bottom)
        let scrollPoint = NSPoint(
            x: 0,
            y: contentSize.height
        )

        // Scroll to the calculated point
        self.scroll(
            scrollPoint
        )
    }

    // Method 5: Complete solution with proper handling
    public func scrollToTextEnd() {
        guard let textStorage = self.textStorage else {
            return
        }

        // Get the range of the last character
        let length = textStorage.length
        if length > 0 {
            let range = NSRange(
                location: length - 1,
                length: 1
            )
            self.scrollRangeToVisible(
                range
            )
        } else {
            // If empty text, scroll to beginning
            self.scrollRangeToVisible(
                NSRange(
                    location: 0,
                    length: 0
                )
            )
        }
    }

    public func scrollToTextViewEnd() {
        guard let layoutManager = self.layoutManager,
            let textContainer = self.textContainer
        else {
            return
        }
        let _: Bool = layoutManager.isProxy() /* fix swift bullshit warning */

        // Ensure layout is up to date
        layoutManager
            .ensureLayout(
                for: textContainer
            )

        // Get the used rectangle (actual text layout area)
        let usedRect = layoutManager.usedRect(
            for: textContainer
        )

        // Calculate the bottom position (this is more accurate)
        let contentHeight = max(
            0,
            usedRect.height
        )
        let visibleHeight = max(
            1,
            self.visibleRect.height
        )
        let rawScrollY = contentHeight - visibleHeight

        // Clamp to valid scroll range
        let scrollYfix = max(
            0,
            min(
                rawScrollY,
                CGFloat.greatestFiniteMagnitude
            )
        )

        // Scroll to the calculated point
        self.scroll(
            NSPoint(
                x: 0,
                y: scrollYfix
            )
        )

        // Additional safety - ensure we're at the very end
        if let textStorage = self.textStorage {
            let endRange = NSRange(
                location: textStorage.length,
                length: 0
            )
            self.scrollRangeToVisible(
                endRange
            )
        }
    }

    // Format text with specific background color, foreground color, and font
    // - Parameters:
    //   - text: The text to format
    //   - backgroundColor: Background color for the text
    //   - textColor: Foreground color (text color)
    //   - fontName: Font name (e.g., "Menlo", "Monaco")
    //   - fontSize: Font size
    public func setText(
        _ text: String,
        backgroundColor: NSColor = .clear,
        textColor: NSColor = .white,
        fontName: String = "Menlo",
        fontSize: CGFloat = 15.0
    ) {

        // Create attributes dictionary
        let attributes: [NSAttributedString.Key: Any] = [
            .backgroundColor: backgroundColor,
            .foregroundColor: textColor,
            .font:
                NSFont
                .monospacedSystemFont(
                    ofSize: fontSize,
                    weight: .regular
                ),
        ]

        // Create attributed string
        let attributedString = NSAttributedString(
            string: text,
            attributes: attributes
        )

        // Set the attributed string to the text view
        if attributedString.length > 0 {
            self.textStorage?
                .setAttributedString(
                    attributedString
                )
        }
        // Update text view frame and container size to match text width
        if let scrollView = self.enclosingScrollView {
            let contentWidth = scrollView.contentSize.width + 15
            self.frame.size.width = max(
                contentWidth,
                scrollView.contentSize.width
            )
            textContainer?.containerSize = CGSize(
                width: Swift.Double.greatestFiniteMagnitude,
                height: Swift.Double.greatestFiniteMagnitude
            )
        }
        // Force layout recalculation to update horizontal scroller
        self.layoutManager?
            .ensureLayout(
                for: textContainer!
            )
        self.invalidateIntrinsicContentSize()
        self.needsDisplay = true
    }

    public func setLineWrapping(
        _ enableWrap: Bool
    ) {
        guard let scrollView = self.enclosingScrollView else {
            return
        }

        if enableWrap {
            // Ensure the text container wraps at character boundaries
            self.textContainer?.lineBreakMode = .byCharWrapping
            self.textContainer?.widthTracksTextView = true
            self.textContainerInset = .zero

            // Match content width to the scroll view’s visible width
            let contentWidth = scrollView.contentSize.width + 15
            self.frame = CGRect(
                x: 0,
                y: 0,
                width: contentWidth,
                height: 0
            )
            self.textContainer?.containerSize = CGSize(
                width: contentWidth,
                height: .greatestFiniteMagnitude
            )

            // Disable horizontal scrolling for wrapping mode
            scrollView.hasHorizontalScroller = false
            self.isHorizontallyResizable = false
            self.autoresizingMask = [.width]
        } else {
            // Disable wrapping — allow full text expansion
            self.textContainer?.lineBreakMode = .byClipping
            self.textContainer?.widthTracksTextView = false
            self.textContainer?.containerSize = CGSize(
                width: Swift.Double.greatestFiniteMagnitude,
                height: Swift.Double.greatestFiniteMagnitude
            )

            // Allow both directions to resize
            self.isVerticallyResizable = true
            self.isHorizontallyResizable = true
            self.autoresizingMask = [
                .width,
                .height,
            ]

            scrollView.hasHorizontalScroller = true
            scrollView.autoresizingMask = [
                .width,
                .height,
            ]

            // Force layout recalculation to update horizontal scroller
            self.layoutManager?
                .ensureLayout(
                    for: textContainer!
                )
            self.invalidateIntrinsicContentSize()
            self.needsDisplay = true
        }
    }
}

// MARK: - NSView Extension

extension NSView {

    // Enables or disables all NSControl-based subviews recursively.
    // - Parameter enabled: Pass `true` to enable controls, `false` to disable.
    public func setControlsEnabled(
        _ enabled: Bool
    ) {
        // If this view is a control (e.g., NSButton, NSTextField), toggle its enabled state
        if let control = self as? NSControl {
            control.isEnabled = enabled
        }
        // Recursively update all child views
        for subview in subviews {
            subview
                .setControlsEnabled(
                    enabled
                )
        }
    }

    // Returns `true` if all NSControl-based subviews are enabled, `false` otherwise.
    // (Checks recursively.)
    public func areAllControlsEnabled() -> Bool {
        if let control = self as? NSControl, control.isEnabled == false {
            return false
        }
        for subview in subviews {
            if subview
                .areAllControlsEnabled() == false
            {
                return false
            }
        }
        return true
    }

    // Convenient computed property for enabling/disabling all controls.
    public var isViewEnabled: Bool {
        get {
            return areAllControlsEnabled()
        }
        set {
            setControlsEnabled(
                newValue
            )
        }
    }
}

// MARK: - NSWindow Extension

extension NSWindow {

    private var progressIndicator: NSProgressIndicator? {
        get {
            objc_getAssociatedObject(
                self,
                &progressIndicatorKey
            ) as? NSProgressIndicator
        }
        set {
            objc_setAssociatedObject(
                self,
                &progressIndicatorKey,
                newValue,
                .OBJC_ASSOCIATION_RETAIN_NONATOMIC
            )
        }
    }

    private var progressOverlayView: NSView? {
        get {
            objc_getAssociatedObject(
                self,
                &progressOverlayKey
            ) as? NSView
        }
        set {
            objc_setAssociatedObject(
                self,
                &progressOverlayKey,
                newValue,
                .OBJC_ASSOCIATION_RETAIN_NONATOMIC
            )
        }
    }

    // Shows a centered, indeterminate progress indicator on the window
    public func showProgress() {
        // Avoid duplicates
        if progressIndicator != nil {
            return
        }

        guard let contentView = self.contentView else {
            return
        }

        // Create overlay view with 50% opacity
        let overlayView = NSView()
        overlayView.wantsLayer = true
        overlayView.layer?.backgroundColor =
            NSColor.black
            .withAlphaComponent(
                0.5
            ).cgColor
        overlayView.translatesAutoresizingMaskIntoConstraints = false

        let indicator = NSProgressIndicator()
        indicator.style = .spinning
        indicator.controlSize = .regular
        indicator.isIndeterminate = true
        indicator.usesThreadedAnimation = true
        indicator
            .startAnimation(
                nil
            )
        indicator.translatesAutoresizingMaskIntoConstraints = false

        // Add progress indicator to overlay view
        overlayView
            .addSubview(
                indicator
            )

        // Add overlay on top
        contentView
            .addSubview(
                overlayView
            )

        // Set up constraints for overlay view (full size of content view)
        NSLayoutConstraint
            .activate(
                [
                    overlayView.topAnchor
                        .constraint(
                            equalTo: contentView.topAnchor
                        ),
                    overlayView.leadingAnchor
                        .constraint(
                            equalTo: contentView.leadingAnchor
                        ),
                    overlayView.trailingAnchor
                        .constraint(
                            equalTo: contentView.trailingAnchor
                        ),
                    overlayView.bottomAnchor
                        .constraint(
                            equalTo: contentView.bottomAnchor
                        ),
                ]
            )

        // Set up constraints for progress indicator (centered)
        NSLayoutConstraint
            .activate(
                [
                    indicator.centerXAnchor
                        .constraint(
                            equalTo: overlayView.centerXAnchor
                        ),
                    indicator.centerYAnchor
                        .constraint(
                            equalTo: overlayView.centerYAnchor
                        ),
                ]
            )

        self.progressIndicator = indicator
        self.progressOverlayView = overlayView
    }

    // Updates a determinate progress indicator’s value.
    public func updateProgress(
        to value: Double
    ) {
        guard let indicator = progressIndicator else {
            return
        }

        if indicator.isIndeterminate {
            indicator.isIndeterminate = false
            indicator.minValue = 0.0
            indicator.maxValue = 1.0
        }
        indicator.doubleValue = value
    }

    // Hides and removes the progress indicator.
    public func hideProgress() {
        guard let indicator = progressIndicator else {
            return
        }

        indicator
            .stopAnimation(
                nil
            )

        // Remove the entire container view (which
        // includes both overlay and indicator)
        if let overlayView = progressOverlayView {
            // Remove the overlay view first
            overlayView
                .removeFromSuperview()
        }

        // Remove the indicator (in case it's not already removed)
        indicator
            .removeFromSuperview()

        progressIndicator = nil
        progressOverlayView = nil
    }
}

// MARK: - NSViewController Extension

extension NSViewController {
    public func showProgress() {
        view.window?
            .showProgress()
    }

    public func updateProgress(
        to value: Double
    ) {
        view.window?
            .updateProgress(
                to: value
            )
    }

    public func hideProgress() {
        view.window?
            .hideProgress()
    }

    private var viewObserverToken: Any? {
        get {
            objc_getAssociatedObject(
                self,
                &viewControllerKey
            )
        }
        set {
            objc_setAssociatedObject(
                self,
                &viewControllerKey,
                newValue,
                .OBJC_ASSOCIATION_RETAIN_NONATOMIC
            )
        }
    }

    // MARK: - Window Close Handler

    // Add window close observer with callback
    func addWindowCloseObserver(
        completion: @escaping () -> Void
    ) {
        // Store the observer token to prevent memory leaks
        let token = NotificationCenter.default.addObserver(
            forName: NSWindow.willCloseNotification,
            object: nil,
            queue: .main
        ) { notification in
            // Check if the window belongs to this view controller
            guard let window = notification.object as? NSWindow,
                let viewController = window.contentViewController,
                viewController == self
            else {
                return
            }

            completion()
        }

        // Store the token in a property (you might want
        // to use a more robust storage solution)
        self.viewObserverToken = token
    }

    // Remove window close handler
    func removeWindowCloseHandler() {
        if let observer = viewObserverToken {
            NotificationCenter.default
                .removeObserver(
                    observer
                )
        }
    }

    // Add window close handler with completion closure
    func onWindowClose(completion: @escaping () -> Void) {
        addWindowCloseObserver(
            completion: completion
        )
    }

    func setupWillCloseHandler() {
        // Add observer for window will close notification
        NotificationCenter.default
            .addObserver(self, selector: #selector(windowWillClose(_:)),
                name: NSWindow.willCloseNotification,
                object: nil
            )
    }

    @objc private func windowWillClose(_ notification: Notification) {
        // This will be called when any window is about to close
        // You can filter by specific window if needed

        guard let window = notification.object as? NSWindow else {
            return
        }

        // Check if this view controller's window is closing
        if let viewController = window.contentViewController,
            viewController == self
        {
            viewWillClose()
        }
    }

    // Override this method in your subclass to handle window close
    @objc func viewWillClose() {
        // Default implementation - override in subclasses
    }
}

// MARK: - NSWindowController Extension

extension NSWindowController {
    public func showProgress() {
        window?.showProgress()
    }

    public func updateProgress(to value: Double) {
        window?.updateProgress(to: value)
    }

    public func hideProgress() {
        window?.hideProgress()
    }
}

// MARK: Extension to NSTextField

extension NSTextField {
    private var editor: NSText? {
        return self.currentEditor()
    }

    open override func performKeyEquivalent(with event: NSEvent) -> Bool {
        guard let characters = event.charactersIgnoringModifiers else {
            return super.performKeyEquivalent(with: event)
        }
        guard let editor = editor else {
            return super.performKeyEquivalent(
                with: event
            )
        }

        // ⌘C, ⌘X, ⌘V and ⌘Backspace
        if event.modifierFlags.contains(.command)
        {
            switch characters.lowercased()
            {
                case "c":
                    editor.copy(nil)
                    return true
                case "x":
                    if editor.isEditable {
                        editor.cut(nil)
                    }
                    return true
                case "v":
                    if editor.isEditable {
                        editor.paste(nil)
                    }
                    return true
                case String(UnicodeScalar(NSDeleteCharacter)!):
                    if editor.isEditable {
                        self.stringValue = ""
                    }
                    return true
                default:
                    break
            }
        }

        return super.performKeyEquivalent(
            with: event
        )
    }
}

// MARK: Extension to NSTextView

extension NSTextView {

    open override func performKeyEquivalent(with event: NSEvent) -> Bool {
        guard let characters = event.charactersIgnoringModifiers else {
            return super.performKeyEquivalent(with: event)
        }

        // ⌘C, ⌘X, ⌘V and ⌘Backspace
        if event.modifierFlags.contains(.command)
        {
            switch characters.lowercased() {
                // Copy
                case "c":
                    self.copy(nil)
                    return true
                // Cut
                case "x":
                    if self.isEditable {
                        self.cut(nil)
                    }
                    return true
                // Paste
                case "v":
                    if self.isEditable {
                        self.paste(nil)
                    }
                    return true
                // force delete content anyway
                case String(UnicodeScalar(NSDeleteCharacter)!):
                    self.string = ""
                    return true
                default:
                    break
            }
        }

        return super.performKeyEquivalent(
            with: event
        )
    }
}

// MARK: Extension to add system symbol image support

extension NSImage {
    //convenience init?(systemSymbolName: String, accessibilityDescription: String?) {
    //    self.init(systemSymbolName: systemSymbolName, accessibilityDescription: accessibilityDescription)
    //}
}

// MARK: Extension to filter system files in open/save dialog

extension SPTestViewController: NSOpenSavePanelDelegate {
    // Filter out system files
    func panel(_ sender: Any,shouldEnable url: URL) -> Bool {
        return !url.path.startsWith(".")
    }
}

// MARK: Extension NSApplication

extension NSApplication {
    var isDarkMode: Bool {
        return effectiveAppearance.name == .darkAqua
    }
    var isLightMode: Bool {
        return effectiveAppearance.name == .aqua
    }
}
