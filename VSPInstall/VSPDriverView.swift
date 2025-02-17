// ********************************************************************
// VSPDriverView.swift - VSP setup app
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
import SwiftUI

struct VSPDriverView: View {

    enum DisplayedView {
        case stateManagement
        case communication
    }

    @State var displayedView: DisplayedView = .stateManagement

    var body: some View {
        switch displayedView {
        case .stateManagement:
            VSPLoaderView(displayedView: $displayedView)
        case .communication:
            VSPTestView(displayedView: $displayedView)
        }
    }
}
