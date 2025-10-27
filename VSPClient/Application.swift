//
//  Application.swift
//  VCPClient
//
//  Created by Björn Eschrich on 26.10.25.
//

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
