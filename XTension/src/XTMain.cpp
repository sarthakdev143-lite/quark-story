/**
 * XTMain.cpp - Quark XTension Entry Point
 * 
 * Production-grade XTension implementation with:
 * - Proper SDK integration
 * - Clean resource management
 * - Comprehensive logging
 * - Error handling
 * 
 * @author Senior Developer
 * @version 1.0.0
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "StoryPalette.h"
#include "Logger.h"

// Forward declare Quark SDK types to avoid dependency on missing header
// In production, include the actual SDK header: #include "XTExtensions.h"
typedef long XTError;
typedef int XTExtensionID;
#define STDCALL __stdcall
#define kNoErr 0

extern "C" {
    // Quark SDK function declarations (stubbed for compilation without SDK)
    // These will be resolved when linked against actual Quark SDK
    XTError STDCALL XTRegisterMenuItem(XTExtensionID extID, const char* name, 
                                        void* icon, void (*callback)(void*, void*), void* userData);
}

using namespace StoryXT;

// Global palette instance (managed by XTension lifecycle)
static StoryPalette* g_palette = nullptr;

/**
 * Menu item callback - triggered when user selects our menu item.
 * Creates and shows the story palette on first invocation.
 * 
 * @param menuItem The menu item reference (unused)
 * @param userData User data passed during registration (unused)
 */
static void STDCALL menuCallback(void* menuItem, void* userData) {
    (void)menuItem;  // Suppress unused parameter warning
    (void)userData;  // Suppress unused parameter warning
    
    if (!g_palette) {
        // Obtain Quark's main window handle
        // Note: In production, use XTGetMainWindow() from SDK
        HWND hMain = GetActiveWindow();
        
        try {
            g_palette = new StoryPalette(GetModuleHandle(nullptr), hMain);
            Logger::info("Story palette created via menu callback");
        } catch (const std::exception& e) {
            Logger::error(std::string("Failed to create palette: ") + e.what());
            MessageBoxA(hMain, "Failed to initialize Story Explorer", "Error", MB_ICONERROR);
            return;
        }
    }
    
    if (g_palette) {
        g_palette->Show();
    }
}

/**
 * XTension initialization entry point.
 * Called by QuarkXPress when the XTension is loaded.
 * 
 * @param extID Extension identifier assigned by Quark
 * @return kNoErr on success, error code otherwise
 */
extern "C" __declspec(dllexport) XTError STDCALL XTInit(XTExtensionID extID) {
    // Initialize logging first for diagnostics
    Logger::init("StoryXT.log", LogLevel::INFO);
    Logger::info("========================================");
    Logger::info("QuarkStoryXT initializing");
    Logger::info("Extension ID: " + std::to_string(extID));
    Logger::info("========================================");

    // Register our menu item with Quark
    XTError err = XTRegisterMenuItem(
        extID, 
        "Story Explorer",      // Menu item text
        nullptr,                // Icon (optional)
        menuCallback,           // Callback function
        nullptr                 // User data
    );
    
    if (err != kNoErr) {
        Logger::error("Failed to register menu item. Error code: " + std::to_string(err));
        return err;
    }
    
    Logger::info("Menu item 'Story Explorer' registered successfully");
    Logger::info("QuarkStoryXT initialization complete");
    
    return kNoErr;
}

/**
 * XTension cleanup entry point.
 * Called by QuarkXPress when the XTension is unloaded.
 * 
 * @param extID Extension identifier
 */
extern "C" __declspec(dllexport) void STDCALL XTDeInit(XTExtensionID extID) {
    Logger::info("QuarkStoryXT shutting down");
    Logger::info("Extension ID: " + std::to_string(extID));
    
    // Clean up palette
    if (g_palette) {
        delete g_palette;
        g_palette = nullptr;
        Logger::info("Palette cleaned up");
    }
    
    // Close log file
    Logger::close();
    Logger::info("QuarkStoryXT shutdown complete");
}

/**
 * DLL entry point - handles DLL load/unload events.
 * Used for any additional initialization/cleanup needed at DLL level.
 * 
 * @param hModule Module handle
 * @param ul_reason_for_call Reason for the call
 * @param lpReserved Reserved parameter
 * @return TRUE on success
 */
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    (void)lpReserved;  // Suppress unused parameter warning
    
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            // DLL is being loaded into a process's address space
            DisableThreadLibraryCalls(hModule);
            break;
        case DLL_THREAD_ATTACH:
            // A new thread is being created in the process
            break;
        case DLL_THREAD_DETACH:
            // A thread is exiting cleanly
            break;
        case DLL_PROCESS_DETACH:
            // The DLL is being detached from the process's address space
            // Ensure cleanup if XTDeInit wasn't called
            if (g_palette) {
                delete g_palette;
                g_palette = nullptr;
            }
            Logger::close();
            break;
        default:
            break;
    }
    return TRUE;
}