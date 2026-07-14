/**
 * StoryPalette.h - Story Explorer Palette UI
 * 
 * Production-grade palette implementation with:
 * - Modeless dialog for non-blocking UX
 * - Background threading for API calls
 * - Thread-safe UI updates via message passing
 * - Proper resource management
 * 
 * @author Senior Developer
 * @version 1.0.0
 */

#pragma once

#include <windows.h>
#include <memory>
#include <thread>
#include <vector>
#include <atomic>
#include "ApiClient.h"

namespace StoryXT {

/**
 * Manages the story explorer palette window.
 * Handles displaying story lists, user interactions, and detail views.
 */
class StoryPalette {
public:
    /**
     * Construct the story palette.
     * 
     * @param hInstance Application instance handle
     * @param hwndParent Parent window handle (Quark main window)
     */
    StoryPalette(HINSTANCE hInstance, HWND hwndParent);
    
    /**
     * Destructor - ensures proper cleanup of resources.
     */
    ~StoryPalette();

    // Non-copyable
    StoryPalette(const StoryPalette&) = delete;
    StoryPalette& operator=(const StoryPalette&) = delete;

    /**
     * Show the palette window.
     * Triggers automatic story loading.
     */
    void Show();
    
    /**
     * Hide the palette window without destroying it.
     */
    void Hide();
    
    /**
     * Check if the palette is currently visible.
     * @return true if visible, false otherwise
     */
    bool IsVisible() const { return m_hDlg != nullptr && IsWindowVisible(m_hDlg); }
    
    /**
     * Refresh the story list from the API.
     */
    void Refresh();

private:
    /**
     * Dialog procedure for handling Windows messages.
     */
    static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
    
    /**
     * Handle command messages (button clicks, list selection).
     * @param wParam Command parameter
     */
    void OnCommand(WPARAM wParam);
    
    /**
     * Load stories from the API in a background thread.
     */
    void LoadStories();
    
    /**
     * Update the list box with fetched stories.
     * Must be called on UI thread.
     * @param stories Vector of story summaries to display
     */
    void UpdateList(const std::vector<StorySummary>& stories);
    
    /**
     * Display story details for selected story.
     * @param id Story ID to fetch and display
     */
    void DisplayStoryDetail(int id);
    
    /**
     * Convert UTF-8 string to wide string for Windows UI.
     * @param str UTF-8 encoded string
     * @return Wide string
     */
    static std::wstring ToWideString(const std::string& str);
    
    /**
     * Set loading state for the UI.
     * @param loading true to show loading indicator
     */
    void SetLoadingState(bool loading);

    HINSTANCE m_hInst;                          // Application instance
    HWND m_hParent;                             // Parent window handle
    HWND m_hDlg;                                // Dialog window handle
    std::unique_ptr<ApiClient> m_apiClient;     // API client for data fetching
    std::vector<StorySummary> m_stories;        // Cached story list
    std::atomic<bool> m_loading;                // Loading state flag
    std::atomic<bool> m_initialLoadComplete;    // Track initial load status
};

} // namespace StoryXT