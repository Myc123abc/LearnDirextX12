#pragma once

#include <Windows.h>

namespace Util
{
    // Use windows memory check
    void enableMemCheck();

    // Use for create window
    struct WindowConfig
    {
        HINSTANCE hInstance  = nullptr;
        PCWSTR    className  = nullptr;
        PCWSTR    windowName = nullptr;
        int       x          = CW_USEDEFAULT;
        int       y          = CW_USEDEFAULT;
        int       width      = CW_USEDEFAULT;
        int       height     = CW_USEDEFAULT;
        DWORD     dwStyle    = WS_OVERLAPPEDWINDOW;
        DWORD     dwExStyle  = WS_EX_LEFT;
        HWND      hWndParent = nullptr;
        HMENU     hMenu      = nullptr;
        void*     userData   = nullptr;
        decltype(WNDCLASSEXW::lpfnWndProc) wndProc = DefWindowProcW;
    };

    HWND createWindow(WindowConfig& config) noexcept;

    bool showWindow(const HWND hWnd) noexcept;

}