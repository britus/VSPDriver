// ********************************************************************
// VSPInstallApp.swift - VSP setup app
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
import Foundation
import AppKit

@main
struct VSPInstallApp {
    static func main() {
        if (isArchArm64() || isQt5()) {
            QT_StartApplication()
        }
        else {
            showMessage(
                "Virtual Serial Port Controller",
                "Unfortunately, this application support Apple M chips on ARM64 architecture.\n" +
                "You can use older releases as Intel® 64bit build from our repository on GitHub.\n" +
                "For older releases visit: https://github.com/britus/VSPClient/releases\n" +
                "\nSorry for the inconvenience and thank you for your understanding.\n");
        }
    }
 }
