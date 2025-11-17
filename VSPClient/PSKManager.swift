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
    
    static public let shared: PSKManager = PSKManager()
    @Published @objc dynamic private(set) var isRestricted: Bool = true

    private var observers = NSHashTable<AnyObject>.weakObjects()
    
    public func addObserver(_ observer: PSKManagerObserver) {
        observers.add(observer)
    }
    
    public func removeObserver(_ observer: PSKManagerObserver) {
        observers.remove(observer)
    }

    public func query()
    {
        Task {
            await validateReceipt()
        }
    }

    public func refresh()
    {
        Task {
            await refreshReceipt()
        }
    }

    private func notifyObservers(_ verified: Bool)
    {
        isRestricted = (verified == false)
        for observer in observers.allObjects {
            (observer as? PSKManagerObserver)?
                .statusChanged(verified)
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
                    handleUnverifiedTransaction(appTxn: appTxn, error: error)
                    break
            }
        }
        catch {
            NSLog("[PSKMGR] Error fetching AppTransaction: \(error)")
            notifyObservers(false)
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
                    handleUnverifiedTransaction(appTxn: appTxn, error: error)
                    break
            }
        }
        catch {
            NSLog("[PSKMGR] Error fetching AppTransaction: \(error)")
            notifyObservers(false)
        }
    }
    
    private func handleVerifiedTransaction(_ txn: AppTransaction)
    {
        //NSLog("Verified App Transaction:")
        //NSLog(" Original App Version: \(txn.originalAppVersion)")
        //NSLog(" Original Purchase Date: \(txn.originalPurchaseDate)")
        //NSLog(" Environment: \(txn.environment.rawValue)")
        notifyObservers(true)
    }

    private func handleUnverifiedTransaction(appTxn: AppTransaction, error: VerificationResult<AppTransaction>.VerificationError)
    {
        //NSLog("Unverified App Transaction:")
        //NSLog(" Reason: \(error.localizedDescription)")
        //NSLog(" Original App Version: \(appTxn.originalAppVersion)")
        //NSLog(" Original Purchase Date: \(appTxn.originalPurchaseDate)")
        notifyObservers(false)
    }

    @MainActor public func review(_ vc: NSViewController)
    {
        AppStore.requestReview(in: vc)
    }
}
