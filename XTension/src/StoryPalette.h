#pragma once
#include <windows.h>
#include <memory>
#include <thread>
#include <vector>
#include "ApiClient.h"

class StoryPalette {
public:
    StoryPalette(HINSTANCE hInstance, HWND hwndParent);
    ~StoryPalette();

    void Show();
    void Hide();

private:
    static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
    void OnCommand(WPARAM wParam);
    void LoadStories();
    void UpdateList(const std::vector<StorySummary>& stories);
    void DisplayStoryDetail(int id);

    HINSTANCE m_hInst;
    HWND m_hParent;
    HWND m_hDlg;
    std::unique_ptr<ApiClient> m_apiClient;
    std::vector<StorySummary> m_stories;
    bool m_loading;
};