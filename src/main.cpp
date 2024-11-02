#include <Windows.h>
#include <crtdbg.h>

#include <exception>

#include "DirectX12.hpp"


HWND createWindow(const wchar_t* name, int width, int height);
// Call after creating DirectX resources
void updateWindow(HWND hWnd);
LRESULT CALLBACK wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
#ifdef _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    int width  = 800;
    int height = 600;

    auto hWnd = createWindow(L"LearnDirectX12", width, height);
    DirectX12 dx12(hWnd, width, height);
    updateWindow(hWnd);

    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    
        dx12.render();
    }
}

HWND createWindow(const wchar_t* name, int width, int height)
{
    auto hInstance = GetModuleHandleW(nullptr);

    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.hInstance     = hInstance;
    wc.lpszClassName = name;
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = wndProc;
    if (RegisterClassExW(&wc) == 0)
        throw std::exception("RegisterClassExW Error");

    // Adjust real size, include title bar
    RECT rect = { 0, 0, width, height };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
    width  = rect.right - rect.left;
    height = rect.bottom - rect.top;

    auto hWnd = CreateWindowW(
        name, nullptr,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        nullptr, nullptr, hInstance, nullptr
    );
    if (hWnd == nullptr)
        throw std::exception("CreateWindowW Error"); 

    return hWnd;
}

void updateWindow(HWND hWnd)
{
    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
}

LRESULT CALLBACK wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_MENUCHAR:
        // Disable beep when Alt + Enter
        return MAKELRESULT(0, MNC_CLOSE);
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}