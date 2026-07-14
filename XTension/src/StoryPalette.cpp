#include "StoryPalette.h"
#include "resource.h"
#include "Logger.h"
#include <sstream>
#include <codecvt>
#include <locale>

// Convert UTF-8 string to wide string (Windows)
static std::wstring to_wstring(const std::string& str) {
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], size_needed);
    return wstr;
}

StoryPalette::StoryPalette(HINSTANCE hInstance, HWND hwndParent)
    : m_hInst(hInstance), m_hParent(hwndParent), m_hDlg(nullptr), m_loading(false) {
    m_apiClient = std::make_unique<ApiClient>("http://localhost:5000");
    // Create modeless dialog
    CreateDialogParam(hInstance, MAKEINTRESOURCE(IDD_STORY_PALETTE), hwndParent, DlgProc, reinterpret_cast<LPARAM>(this));
}

StoryPalette::~StoryPalette() {
    if (m_hDlg) DestroyWindow(m_hDlg);
}

void StoryPalette::Show() {
    if (m_hDlg) {
        ShowWindow(m_hDlg, SW_SHOW);
        // Trigger data fetch when shown
        std::thread(&StoryPalette::LoadStories, this).detach();
    }
}

void StoryPalette::Hide() {
    if (m_hDlg)
        ShowWindow(m_hDlg, SW_HIDE);
}

INT_PTR CALLBACK StoryPalette::DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    StoryPalette* pThis = nullptr;
    if (msg == WM_INITDIALOG) {
        SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
        pThis = reinterpret_cast<StoryPalette*>(lParam);
        pThis->m_hDlg = hDlg;
        return TRUE;
    }
    pThis = reinterpret_cast<StoryPalette*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
    if (!pThis) return FALSE;

    switch (msg) {
        case WM_COMMAND:
            pThis->OnCommand(wParam);
            return TRUE;
        case WM_CLOSE:
            pThis->Hide();
            return TRUE;
        case WM_USER: {
            // Received from worker thread: update the list
            if (lParam) {
                auto* stories = reinterpret_cast<std::vector<StorySummary>*>(lParam);
                pThis->UpdateList(*stories);
                delete stories;
            }
            return TRUE;
        }
    }
    return FALSE;
}

void StoryPalette::OnCommand(WPARAM wParam) {
    WORD id = LOWORD(wParam);
    if (id == IDC_BTN_REFRESH) {
        std::thread(&StoryPalette::LoadStories, this).detach();
    } else if (id == IDC_LIST_STORIES && HIWORD(wParam) == LBN_SELCHANGE) {
        int sel = static_cast<int>(SendDlgItemMessage(m_hDlg, IDC_LIST_STORIES, LB_GETCURSEL, 0, 0));
        if (sel >= 0 && sel < static_cast<int>(m_stories.size())) {
            int storyId = m_stories[sel].id;
            std::thread(&StoryPalette::DisplayStoryDetail, this, storyId).detach();
        }
    }
}

void StoryPalette::LoadStories() {
    m_loading = true;
    auto result = m_apiClient->fetchStories();
    if (result) {
        m_stories = std::move(*result);
        // Marshal to UI thread
        PostMessage(m_hDlg, WM_USER, 0, reinterpret_cast<LPARAM>(new std::vector<StorySummary>(m_stories)));
    } else {
        MessageBox(m_hDlg, L"Failed to load stories.\nIs the backend running?", L"Error", MB_ICONERROR);
        Logger::error("Failed to load stories from API");
    }
    m_loading = false;
}

void StoryPalette::UpdateList(const std::vector<StorySummary>& stories) {
    HWND listBox = GetDlgItem(m_hDlg, IDC_LIST_STORIES);
    SendMessage(listBox, LB_RESETCONTENT, 0, 0);
    for (const auto& s : stories) {
        std::wstring entry = to_wstring(s.title + " by " + s.author);
        SendMessage(listBox, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(entry.c_str()));
    }
}

void StoryPalette::DisplayStoryDetail(int id) {
    auto detail = m_apiClient->fetchStoryDetail(id);
    if (detail) {
        // Show in a message box for demonstration (in production, use a custom detail pane)
        std::string content = detail->content;
        std::wstring wtitle = to_wstring(detail->title);
        std::wstring wcontent = to_wstring(content);
        MessageBoxW(m_hDlg, wcontent.c_str(), wtitle.c_str(), MB_OK);
        Logger::info("Displayed story ID " + std::to_string(id));
    } else {
        MessageBox(m_hDlg, L"Failed to retrieve story details.", L"Error", MB_ICONERROR);
    }
}