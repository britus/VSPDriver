// ********************************************************************
// VSPClient User Interface
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
import Foundation
import AppKit

//  Manages a persistent history of file URLs using security-scoped bookmarks.
//  Works inside macOS sandboxed AppKit applications.
final class FileHistory {

    static let shared = FileHistory()

    private let historyKey = "VSPScriptFile.History"
    private var bookmarkDataList: [Data] = []

    public var isEmpty : Bool {
        return bookmarkDataList.isEmpty
    }
    
    private init() {
        loadHistory()
    }

    // MARK: - Public API

    //  Opens a file picker and adds the selected file URL to the history.
    func openFileAndAddToHistory() -> URL? {
        let panel = NSOpenPanel()
        panel.allowsMultipleSelection = false
        panel.canChooseFiles = true
        panel.canChooseDirectories = false
        panel.title = "Select File"
        
        // Allow multiple JavaScript-related file types
        panel.allowedContentTypes = [.javaScript,.script,.text]

        guard panel.runModal() == .OK, let url = panel.url else {
            return nil
        }
        
        return addToHistory(url)
    }

    //  Returns all URLs from the stored bookmark list.
    func allHistoryURLs() -> [URL] {
        return bookmarkDataList.compactMap { data in
            var isStale = false
            do {
                let url = try URL(
                    resolvingBookmarkData: data,
                    options: [.withSecurityScope],
                    relativeTo: nil,
                    bookmarkDataIsStale: &isStale
                )
                if isStale {
                    _ = refreshBookmark(for: url)
                }
                return url
            } catch {
                showError(for: nil, message: "Failed to resolve bookmark:\n\(error.localizedDescription)")
                return nil
            }
        }
    }

    //  Opens the file at the given history index using the default system editor.
    func openFile(at index: Int) {
        guard let url = url(at: index) else { return }

        if url.startAccessingSecurityScopedResource() {
            NSWorkspace.shared.open(url)
            url.stopAccessingSecurityScopedResource()
        } else {
            showError(for: url, message: "Unable to open file:\n\(url.relativePath)")
        }
    }

    //  Returns the URL at a given index, or nil if the index is invalid.
    func url(at index: Int) -> URL? {
        let urls = allHistoryURLs()
        guard index >= 0 && index < urls.count else { return nil }
        return urls[index]
    }

    //  Returns the index of a given URL if it exists in the history.
    func indexOfUrl(_ url: URL) -> Int? {
        let urls = allHistoryURLs().map { $0.standardizedFileURL }
        return urls.firstIndex(of: url.standardizedFileURL)
    }

    //  Clears all stored entries from the history.
    func clearHistory() {
        bookmarkDataList.removeAll()
        saveHistory()
    }

    //  Removes a specific URL from the history.
    func removeUrl(forKey url: URL) {
        bookmarkDataList.removeAll { data in
            var isStale = false
            if let storedURL = try? URL(
                resolvingBookmarkData: data,
                options: [.withSecurityScope],
                relativeTo: nil,
                bookmarkDataIsStale: &isStale
            ) {
                return storedURL.standardizedFileURL == url.standardizedFileURL
            }
            return false
        }
        saveHistory()
    }

    //  Adds a new URL to the history and stores its security-scoped bookmark.
    public func addToHistory(_ url: URL) -> URL? {
        // prevent duplicates...
        if let _ = indexOfUrl(url) {
            return url
        }
        // --
        do {
            let bookmark = try url.bookmarkData(
                options: [.withSecurityScope],
                includingResourceValuesForKeys: nil,
                relativeTo: nil
            )
            bookmarkDataList.append(bookmark)
            saveHistory()
        } catch {
            showError(for: url, message: "Failed to create bookmark:\n\(error.localizedDescription)")
            return nil
        }
        return url
    }

    //  Updates the bookmark for a given URL if it has gone stale.
    public func refreshBookmark(for url: URL) -> URL? {
        do {
            let newBookmark = try url.bookmarkData(
                options: [.withSecurityScope],
                includingResourceValuesForKeys: nil,
                relativeTo: nil
            )
            if let index = bookmarkDataList.firstIndex(where: { oldData in
                var isStale = false
                if let resolved = try? URL(
                    resolvingBookmarkData: oldData,
                    options: [.withSecurityScope],
                    relativeTo: nil,
                    bookmarkDataIsStale: &isStale
                ) {
                    return resolved.standardizedFileURL == url.standardizedFileURL
                }
                return false
            }) {
                bookmarkDataList[index] = newBookmark
                saveHistory()
            }
            return url
        } catch {
            showError(for: url, message: "Failed to refresh bookmark:\n\(error.localizedDescription)")
            return nil
        }
    }
    
    // MARK: - Private Helpers

    //  Saves the bookmark data list to UserDefaults.
    private func saveHistory() {
        UserDefaults.standard.set(bookmarkDataList, forKey: historyKey)
    }

    //  Loads the bookmark data list from UserDefaults.
    private func loadHistory() {
        guard let stored = UserDefaults.standard.array(forKey: historyKey) as? [Data] else {
            bookmarkDataList = []
            return
        }
        bookmarkDataList = stored
    }

    //  Shows a UI alert and removes the URL from the history on dismissal.
    private func showError(for url: URL?, message: String) {
        UITools.showAlert(
            title: UITools.applicationName(),
            message: message,
            completion: { [weak self] in
                guard let self = self, let url = url else { return }
                self.removeUrl(forKey: url)
            }
        )
    }
}
