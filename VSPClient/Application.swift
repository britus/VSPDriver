// ********************************************************************
// VSPClient User Interface
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
import Cocoa

class Application: NSApplication {

}

extension NSApplication {
    var isDarkMode: Bool {
        return effectiveAppearance.name == .darkAqua
    }
    var isLightMode: Bool {
        return effectiveAppearance.name == .aqua
    }
}
