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
        if (isArchArm64()) {
            QT_StartApplication()
        }
        else {
            showMessage(
                "Virtual Serial Port Controller",
                "Unfortunately, this application support Apple M chips on ARM64 architecture.\n" +
                "You can build you own App with Intel® 64bit architecture from our source repository on GitHub.\n" +
                "\nThank you for your understanding.\n");
        }
    }
 }
