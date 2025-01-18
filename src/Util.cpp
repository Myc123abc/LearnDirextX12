#include "Util.hpp"
#include <crtdbg.h>

using namespace Util;

void Util::enableMemCheck()
{
#if defined(DEBUG) | defined (_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
}

HWND Util::createWindow(WindowConfig& config) noexcept
{
    if (config.hInstance == nullptr)
    {
        config.hInstance = GetModuleHandleW(nullptr);
        if (config.hInstance == nullptr)
            return nullptr;
    }

    WNDCLASSEXW wc   = {};
    wc.cbSize        = sizeof(wc);
    wc.hInstance     = config.hInstance;
    wc.lpszClassName = config.className;
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = config.wndProc;
    wc.hIcon         = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor       = LoadCursor(0, IDC_ARROW);
    if (RegisterClassExW(&wc) == 0)
        return nullptr;

    if (config.width != CW_USEDEFAULT && config.height != CW_USEDEFAULT)
    {
        RECT rect = { 0, 0, config.width, config.height };
        if (AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false) == FALSE)
            return nullptr;
        config.width  = rect.right - rect.left;
        config.height = rect.bottom - rect.top;
    }

    return CreateWindowExW(config.dwExStyle, config.className, config.windowName, config.dwStyle,
                           config.x, config.y, config.width, config.height,
                           config.hWndParent, config.hMenu, config.hInstance, config.userData);
}

bool Util::showWindow(const HWND hWnd) noexcept
{
    ShowWindow(hWnd, SW_SHOW);
    return UpdateWindow(hWnd);
}