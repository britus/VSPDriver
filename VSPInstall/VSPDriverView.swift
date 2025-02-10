/* See the LICENSE.txt file for this sample’s licensing information.

Abstract:
The SwiftUI view that manages the views of the app.
*/

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
