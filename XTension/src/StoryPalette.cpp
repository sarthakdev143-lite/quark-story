/**
 * StoryPalette.cpp - Story Explorer Palette Implementation
 * 
 * Production-grade implementation with:
 * - Modeless dialog for non-blocking user experience
 * - Background threading for API calls to prevent UI freezing
 * - Thread-safe UI updates via Windows message passing
 * - Proper UTF-8 to wide string conversion for international text
 * - Loading state management
 */

#include "StoryPalette.h"
#include "Logger.h"
#include "resource.h"
#include <sstream>
#include <codecvt>
#include <locale>

namespace StoryXT {

// Custom message ID for UI thread updates (WM_USER + 1)
constexpr UINT WM_STORIES_LOADED = WM_USER + 1;
constexpr UINT WM_STORY_DETAIL_READY = WM_USER + 2;

/**
 * Convert UTF-8 string to wide string for Windows Unicode APIs.
 * @param str UTF-8 encoded source string
 * @return Wide string (UTF-16 on Windows)
 */
static std::wstring ToWideString(const std::string& str) {
    if (str.empty()) return L"";
    
    try {
        int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), 
                                              static_cast<int>(str.size()), NULL, 0);
        if (sizeNeeded == 0) {
            // Fallback for non-UTF8 strings
            return std::wstring(str.begin(), str.end());
        }
        
        std::wstring wstr(sizeNeeded, 0);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), 
                           static_cast<int>(str.size()), &wstr[0], sizeNeeded);
        return wstr;
    } catch (...) {
        return std::wstring(str.begin(), str.end());
    }
}

StoryPalette::StoryPalette(HINSTANCE hInstance, HWND hwndParent)
    : m_hInst(hInstance)
    , m_hParent(hwndParent)
    , m_hDlg(nullptr)
    , m_loading(false)
    , m_initialLoadComplete(false) {
    
    m_apiClient = std::make_unique<ApiClient>("http://localhost:5000");
    
    // Create modeless dialog - allows user to continue working in Quark
    // while the palette is open
    HWND hDlg = CreateDialogParam(
        hInstance, 
        MAKEINTRESOURCE(IDD_STORY_PALETTE), 
        hwndParent, 
        DlgProc, 
        reinterpret_cast<LPARAM>(this)
    );
    
    if (hDlg) {
        m_hDlg = hDlg;
        Logger::info("Story palette created successfully");
    } else {
        DWORD error = GetLastError();
        Logger::error("Failed to create story palette. Error code: " + std::to_string(error));
    }
}

StoryPalette::~StoryPalette() {
    if (m_hDlg && IsWindow(m_hDlg)) {
        DestroyWindow(m_hDlg);
    }
    Logger::info("Story palette destroyed");
}

void StoryPalette::Show() {
    if (m_hDlg && IsWindow(m_hDlg)) {
        ShowWindow(m_hDlg, SW_SHOW);
        SetForegroundWindow(m_hDlg);
        
        // Auto-refresh when shown (only if not already loading)
        bool expected = false;
        if (m_loading.compare_exchange_strong(expected, true)) {
            // Start background thread for non-blocking API call
            std::thread(&StoryPalette::LoadStories, this).detach();
        }
    }
}

void StoryPalette::Hide() {
    if (m_hDlg && IsWindow(m_hDlg)) {
        ShowWindow(m_hDlg, SW_HIDE);
    }
}

void StoryPalette::Refresh() {
    if (!m_hDlg || !IsWindow(m_hDlg)) {
        return;
    }
    
    bool expected = false;
    if (m_loading.compare_exchange_strong(expected, true)) {
        std::thread(&StoryPalette::LoadStories, this).detach();
    }
}

INT_PTR CALLBACK StoryPalette::DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    StoryPalette* pThis = nullptr;
    
    // Handle WM_INITDIALOG specially - it passes 'this' pointer via lParam
    if (msg == WM_INITDIALOG) {
        SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
        pThis = reinterpret_cast<StoryPalette*>(lParam);
        if (pThis) {
            pThis->m_hDlg = hDlg;
            
            // Initialize list box with placeholder text
            HWND listBox = GetDlgItem(hDlg, IDC_LIST_STORIES);
            if (listBox) {
                SendMessage(listBox, LB_ADDSTRING, 0, 
                           reinterpret_cast<LPARAM>(L"Click Refresh to load stories"));
            }
        }
        return TRUE;
    }
    
    // Retrieve 'this' pointer for all other messages
    LONG_PTR ptr = GetWindowLongPtr(hDlg, GWLP_USERDATA);
    if (!ptr) {
        return FALSE;
    }
    pThis = reinterpret_cast<StoryPalette*>(ptr);
    
    switch (msg) {
        case WM_COMMAND: {
            pThis->OnCommand(wParam);
            return TRUE;
        }
        
        case WM_CLOSE: {
            pThis->Hide();
            return TRUE;
        }
        
        case WM_STORIES_LOADED: {
            // Worker thread completed - update UI on main thread
            if (lParam) {
                auto* stories = reinterpret_cast<std::vector<StorySummary>*>(lParam);
                pThis->UpdateList(*stories);
                delete stories;
            }
            pThis->m_loading = false;
            pThis->SetLoadingState(false);
            return TRUE;
        }
        
        case WM_STORY_DETAIL_READY: {
            // Story detail fetched - display it
            if (lParam) {
                auto* detail = reinterpret_cast<StoryDetail*>(lParam);
                std::wstring wtitle = ToWideString(detail->title);
                std::wstring wcontent = ToWideString(detail->content);
                
                MessageBoxW(hDlg, wcontent.c_str(), wtitle.c_str(), MB_OK | MB_ICONINFORMATION);
                
                Logger::info("Displayed story: " + detail->title);
                delete detail;
            }
            return TRUE;
        }
        
        case WM_DESTROY: {
            SetWindowLongPtr(hDlg, GWLP_USERDATA, 0);
            return FALSE;
        }
    }
    
    return FALSE;
}

void StoryPalette::OnCommand(WPARAM wParam) {
    WORD id = LOWORD(wParam);
    WORD notification = HIWORD(wParam);
    
    switch (id) {
        case IDC_BTN_REFRESH: {
            Refresh();
            break;
        }
        
        case IDC_LIST_STORIES: {
            if (notification == LBN_SELCHANGE) {
                // Get selected index
                HWND listBox = GetDlgItem(m_hDlg, IDC_LIST_STORIES);
                int sel = static_cast<int>(SendMessage(listBox, LB_GETCURSEL, 0, 0));
                
                if (sel >= 0 && sel < static_cast<int>(m_stories.size())) {
                    int storyId = m_stories[sel].id;
                    
                    // Fetch and display story detail in background thread
                    std::thread(&StoryPalette::DisplayStoryDetail, this, storyId).detach();
                }
            }
            break;
        }
    }
}

void StoryPalette::LoadStories() {
    Logger::debug("Fetching stories from API...");
    
    auto result = m_apiClient->fetchStories();
    
    if (result) {
        m_stories = std::move(*result);
        Logger::info("Successfully loaded " + std::to_string(m_stories.size()) + " stories");
        
        // Marshal to UI thread using PostMessage (non-blocking)
        PostMessage(m_hDlg, WM_STORIES_LOADED, 0, 
                   reinterpret_cast<LPARAM>(new std::vector<StorySummary>(m_stories)));
    } else {
        Logger::error("Failed to fetch stories from API");
        m_loading = false;
        
        // Show error on UI thread
        PostMessage(m_hDlg, WM_STORIES_LOADED, 0, 0);
        
        // Display error message
        MessageBoxW(m_hDlg, 
                   L"Failed to load stories.\n\nPlease ensure the backend service is running\n"
                   L"at http://localhost:5000",
                   L"Connection Error", 
                   MB_ICONERROR | MB_OK);
    }
}

void StoryPalette::UpdateList(const std::vector<StorySummary>& stories) {
    if (!m_hDlg || !IsWindow(m_hDlg)) {
        return;
    }
    
    HWND listBox = GetDlgItem(m_hDlg, IDC_LIST_STORIES);
    if (!listBox) {
        return;
    }
    
    // Clear existing items
    SendMessage(listBox, LB_RESETCONTENT, 0, 0);
    
    if (stories.empty()) {
        SendMessage(listBox, LB_ADDSTRING, 0, 
                   reinterpret_cast<LPARAM>(L"No stories available"));
        return;
    }
    
    // Add each story as "Title by Author"
    for (const auto& story : stories) {
        std::wstring entry = ToWideString(story.title + " by " + story.author);
        SendMessage(listBox, LB_ADDSTRING, 0, 
                   reinterpret_cast<LPARAM>(entry.c_str()));
    }
    
    Logger::debug("Updated list box with " + std::to_string(stories.size()) + " items");
}

void StoryPalette::DisplayStoryDetail(int id) {
    Logger::debug("Fetching details for story ID: " + std::to_string(id));
    
    auto detail = m_apiClient->fetchStoryDetail(id);
    
    if (detail) {
        // Send detail to UI thread for display
        PostMessage(m_hDlg, WM_STORY_DETAIL_READY, 0, 
                   reinterpret_cast<LPARAM>(new StoryDetail(*detail)));
    } else {
        Logger::error("Failed to fetch story detail for ID: " + std::to_string(id));
        
        // Show error on UI thread
        PostMessage(m_hDlg, WM_USER + 3, 0, 0);
    }
}

void StoryPalette::SetLoadingState(bool loading) {
    if (!m_hDlg || !IsWindow(m_hDlg)) {
        return;
    }
    
    HWND refreshBtn = GetDlgItem(m_hDlg, IDC_BTN_REFRESH);
    if (refreshBtn) {
        EnableWindow(refreshBtn, !loading);
    }
    
    HWND listBox = GetDlgItem(m_hDlg, IDC_LIST_STORIES);
    if (listBox) {
        EnableWindow(listBox, !loading);
    }
    
    if (loading) {
        // Update status text
        HWND listBox = GetDlgItem(m_hDlg, IDC_LIST_STORIES);
        if (listBox) {
            SendMessage(listBox, LB_RESETCONTENT, 0, 0);
            SendMessage(listBox, LB_ADDSTRING, 0, 
                       reinterpret_cast<LPARAM>(L"Loading stories..."));
        }
    }
}

} // namespace StoryXT