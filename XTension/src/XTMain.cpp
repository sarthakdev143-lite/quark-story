#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "XTExtensions.h"    // Quark SDK provided header
#include "StoryPalette.h"
#include "Logger.h"

static StoryPalette* g_palette = nullptr;

// Menu callback
static void menuCallback(XTMenuItemRef menuItem, void* userData) {
    if (!g_palette) {
        // Obtain Quark's main window handle (pseudo-code)
        HWND hMain = GetActiveWindow(); // Replace with SDK function: XTGetMainWindow()
        g_palette = new StoryPalette(GetModuleHandle(nullptr), hMain);
    }
    g_palette->Show();
}

// XTension initialisation
extern "C" __declspec(dllexport) XTError STDCALL XTInit(XTExtensionID extID) {
    Logger::init("StoryXT.log");
    Logger::info("QuarkStoryXT initializing");

    XTErr err = XTRegisterMenuItem(extID, "Story Explorer", nullptr, menuCallback, nullptr);
    if (err != kNoErr) {
        Logger::error("Failed to register menu item");
        return err;
    }
    return kNoErr;
}

// XTension cleanup
extern "C" __declspec(dllexport) void STDCALL XTDeInit(XTExtensionID extID) {
    Logger::info("QuarkStoryXT shutting down");
    delete g_palette;
    g_palette = nullptr;
    Logger::close();
}

// DLL entry
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    return TRUE;
}