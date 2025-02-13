/*
See the LICENSE.txt file for this sample’s licensing information.

Abstract:
The SwiftUI view that provides the userclient communication UI.
*/

import SwiftUI

struct VSPTestView: View {

    @Binding var displayedView: VSPDriverView.DisplayedView
    @ObservedObject public var viewModel: VSPTestModel = .init()

    var body: some View {
        VStack {

            headerView

            if viewModel.isConnected {
                VStack(alignment: .leading) {
                    uncheckedView
                    checkedView
                 }
            } else {
                VStack(alignment: .center) {
                    Text("Driver is not connected")
                }
            }

        }.padding().frame(alignment: .center)
    }

    private var headerView: some View {
        VStack(alignment: .center) {
            Text("Driver Communication").font(.title).padding(.top)

            HStack(alignment: .center) {
                Button(
                    action: {
                        displayedView = .stateManagement
                    }, label: {
                        Text("Manage Dext")
                    }
                ).padding([.leading, .trailing, .bottom], nil)
            }

            if viewModel.isConnected {
                Text(viewModel.stateDescription).font(.callout)
            }
        }.padding([.leading, .trailing, .bottom])
    }

    private var uncheckedView: some View {
        VStack(alignment: .leading) {
            Text("Port Controller").font(.title2)

            HStack(alignment: .center) {
                Button(
                    action: {
                        viewModel.doGetPortList()
                    }, label: {
                        Text("Get Port List")
                    }
                ).fixedSize(horizontal: true, vertical: false).frame(width: 120, height: 32).padding([.trailing])

                Button(
                    action: {
                        viewModel.doLinkPorts()
                    },
                    label: {
                        Text("Link Ports")
                    }
                ).fixedSize(horizontal: true, vertical: false).frame(width: 120, height: 32).padding([.trailing])

                Button(
                    action: {
                        viewModel.doUnLinkPorts()
                    },
                    label: {
                        Text("Unlink Ports")
                    }
                ).fixedSize(horizontal: true, vertical: false).frame(width: 120, height: 32).padding([.trailing])
            }
        }.fixedSize(horizontal: true, vertical: true).frame(width: 400, height: 70, alignment: .leading).padding([.bottom,.leading])
    }

    private var checkedView: some View {
        VStack(alignment: .leading) {
            Text("Checked").font(.title2).padding(.top)

            HStack(alignment: .center) {
                Button(
                    action: {
                        viewModel.doEnablePortChecks()
                    },
                    label: {
                        Text("Enable Checks")
                    }
                ).fixedSize(horizontal: true, vertical: false).frame(width: 120, height: 32).padding([.trailing])

                Button(
                    action: {
                        viewModel.doEnablePortTrace()
                    },
                    label: {
                        Text("Enable Traces")
                    }
                ).fixedSize(horizontal: true, vertical: false).frame(width: 120, height: 32).padding([.trailing])
            }
        }.fixedSize(horizontal: true, vertical: true).frame(width: 400, height: 70, alignment: .leading).padding([.bottom,.leading])
    }
}

struct VSPTestView_Previews: PreviewProvider {

    let displayedView: VSPDriverView.DisplayedView = .communication

    static var previews: some View {
        Group {
            VSPTestView(
                displayedView: .constant(.communication),
                viewModel: VSPTestModel(isConnected: false)
            )

            VSPTestView(
                displayedView: .constant(.communication),
                viewModel: VSPTestModel(isConnected: true)
            )
        }

    }
}
