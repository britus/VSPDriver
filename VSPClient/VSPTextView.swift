// ********************************************************************
// VSPClient User Interface
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
import Cocoa

class VSPTextView: NSTextView {
    
    override func keyDown(with event: NSEvent) {
  
        // Check for command key combinations
        if event.modifierFlags.contains(.command) {
            switch event.charactersIgnoringModifiers {
                case "c":
                    // Handle CMD-C (Copy)
                    let selectedRange = self.selectedRange()
                    if selectedRange.length > 0 {
                        NSPasteboard.general.clearContents()
                        if let text = self.textStorage?.attributedSubstring(from: selectedRange) {
                            NSPasteboard.general.setData(
                                try? text.data(from: NSRange(location: 0, length: text.length),
                                           documentAttributes: [
                                            .documentType: NSAttributedString.DocumentType.rtf]),
                                forType: .rtf)
                            return
                        }
                    }
                    break
                case "a":
                    // Handle CMD-A (Select All)
                    self.selectAll(nil)
                    return
                case .none:
                    break
                default:
                    break
            }
        }
        
        // Call super for other key events
        super.keyDown(with: event)
    }

    override func draw(_ dirtyRect: NSRect) {
        super.draw(dirtyRect)

        // Drawing code here.
    }
}
