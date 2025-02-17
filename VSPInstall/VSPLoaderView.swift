// ********************************************************************
// VSPLoaderView.swift - VSP setup app
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
import SwiftUI

struct VSPLoaderView: View {

    @Binding var displayedView: VSPDriverView.DisplayedView
    @ObservedObject var viewModel: VSPLoaderModel = .init()

    var body: some View {
        VStack(alignment: .center) {
            Text("Driver Manager")
                .padding()
                .font(.title)
            Text(self.viewModel.dextLoadingState)
                .multilineTextAlignment(.center)
            HStack {
                Button(
                    action: {
                        self.viewModel.activateMyDext()
                    }, label: {
                        Text("Install Dext")
                    }
                )
                Button(
                    action: {
                        self.viewModel.removeMyDext()
                    }, label: {
                        Text("Remove Dext")
                    }
                )
                Button(
                    action: {
                        displayedView = .communication
                    }, label: {
                        Text("VSP Tester")
                    }
                )
            }
        }.frame(width: 500, height: 200, alignment: .center)
    }
}

struct DriverLoadingView_Previews: PreviewProvider {

    @State var displayedView: VSPDriverView.DisplayedView = .stateManagement

    static var previews: some View {
        VSPLoaderView(displayedView: .constant(.stateManagement))
    }
}
