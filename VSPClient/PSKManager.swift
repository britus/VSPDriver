// ********************************************************************
// VSPClient User Interface
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
import Foundation
import StoreKit

public protocol PSKManagerObserver: AnyObject {
    func statusChanged(_ verified: Bool)
}

public final class PSKManager: NSObject, ObservableObject {
    typealias VerificationError = VerificationResult<AppTransaction>.VerificationError

    struct AppStatus {
        var appVersion: String?
        var purchaseDate: Date?
        var environment: AppStore.Environment?
        var error: VerificationError?
    }
    
    static public let shared: PSKManager = PSKManager()
    
    @Published @objc dynamic private(set) var isRestricted: Bool = true
    @Published dynamic private(set) var appStatus: AppStatus?
    
    private var observers = NSHashTable<AnyObject>.weakObjects()
    
    public func addObserver(_ observer: PSKManagerObserver) {
        observers.add(observer)
    }
    
    public func removeObserver(_ observer: PSKManagerObserver) {
        observers.remove(observer)
    }

    public func query()
    {
        DispatchQueue.global(qos: .background).async {
            Task {
                await self.validateReceipt()
            }
        }
    }

    public func refresh()
    {
        DispatchQueue.global(qos: .background).async {
            Task {
                await self.refreshReceipt()
            }
        }
    }

    private func askUserForRefresh() {
        // retry App verification
        DispatchQueue.main.async {
            let msg =
            "The status of your app cannot be verified in the Apple App Store.\n" + //
            "Would you like to refresh your app status?"
            if UITools.showQuestionDialog(self, msg) {
                self.refresh()
            } else {
                NSApp.terminate(self)
            }
        }
    }

    func errorOccured(_ what: Int, error: Error) {
        NSLog("[PSKMGR] Error getting AppTransaction [\(what)]: \(error)")
        if what == 1 {
            askUserForRefresh()
        } else {
            let error = error as NSError
            let msg = "AppStore refresh error: \(String(format:"0x%x", error.code))"
            UITools.showMessage(message: msg, info: error.localizedDescription) {
                NSApp.terminate(self)
            }
        }
    }

    private func notifyObservers(_ verified: Bool)
    {
        isRestricted = (verified == false)
        
        DispatchQueue.global(qos: .background).async {
            for observer in self.observers.allObjects {
                (observer as? PSKManagerObserver)?
                    .statusChanged(verified)
            }
        }
    }
    
    private func validateReceipt() async
    {
        do {
            let result = try await AppTransaction.shared
            switch result {
                case .verified(let appTxn):
                    handleVerifiedTransaction(appTxn)
                    break
                // Handle unverified case. You still get `appTxn` with data, and an error.
                case .unverified(let appTxn, let error):
                    handleUnverifiedTransaction(1, appTxn: appTxn, error: error)
                    break
            }
        }
        catch {
            errorOccured(1, error: error)
        }
    }

    private func refreshReceipt() async
    {
        do {
            let result = try await AppTransaction.refresh()
            switch result {
            case .verified(let appTxn):
                handleVerifiedTransaction(appTxn)
                break
                // Handle unverified case. You still get `appTxn` with data, and an error.
            case .unverified(let appTxn, let error):
                handleUnverifiedTransaction(2, appTxn: appTxn, error: error)
                break
            }
        }
        catch {
            errorOccured(2, error: error)
        }
    }

    private func handleVerifiedTransaction(_ appTxn: AppTransaction)
    {
        appStatus = AppStatus()
        appStatus?.appVersion = appTxn.originalAppVersion
        appStatus?.purchaseDate = appTxn.originalPurchaseDate
        appStatus?.environment = appTxn.environment
        notifyObservers(true)
    }

    private func handleUnverifiedTransaction(_ what: Int, appTxn: AppTransaction, error: VerificationError)
    {
        appStatus = AppStatus()
        appStatus?.appVersion = appTxn.originalAppVersion
        appStatus?.purchaseDate = appTxn.originalPurchaseDate
        appStatus?.environment = appTxn.environment
        appStatus?.error = error
        notifyObservers(false)
        
        // do not on refresh
        if what == 1 {
            askUserForRefresh()
        } else {
            var info = ""
            let err = error as NSError
            let msg = "AppStore refresh error: \(String(format:"0x%x", err.code)) \(err.description)"
            switch (error) {
            case .invalidCertificateChain:
                info = "Invalid certificate chain"
                break
            case .invalidDeviceVerification:
                info = "Invalid device verification"
                break
            case .invalidEncoding:
                info = "Invalid encoding"
                break
            case .invalidSignature:
                info = "Invalid signature"
                break
            case .missingRequiredProperties:
                info = "Missing required properties"
                break
            case .revokedCertificate:
                info = "Revoked certificate"
                break
            @unknown default:
                info = "Unknwon reason"
                break
            }
            UITools.showMessage(message: msg, info: info) {
                NSApp.terminate(self)
            }
        }
    }

    @MainActor public func review(_ vc: NSViewController)
    {
        AppStore.requestReview(in: vc)
    }
}
