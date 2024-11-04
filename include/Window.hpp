#pragma once

#include <Windows.h>

template <class T>
class BaseWindow
{
public:
    BaseWindow()          = default;
    virtual ~BaseWindow() = default;

    BaseWindow(const BaseWindow&)            = default;
    BaseWindow(BaseWindow&&)                 = default;
    BaseWindow& operator=(const BaseWindow&) = default;
    BaseWindow& operator=(BaseWindow&&)      = default;

    static LRESULT CALLBACK windowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        T* pThis = nullptr;

        if (msg == WM_NCCREATE)
        {
            auto pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
            pThis = reinterpret_cast<T*>(pCreate->lpCreateParams);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));

            pThis->m_hWnd = hWnd;
        }
        else
        {
            pThis = reinterpret_cast<T*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
        }

        if (pThis)
        {
            return pThis->handleMessage(msg, wParam, lParam);
        }
        else 
        {
            return DefWindowProcW(hWnd, msg, wParam, lParam);
        }
    } 

    bool create(
        int    width      = CW_USEDEFAULT,
        int    height     = CW_USEDEFAULT,
        PCWSTR windowName = nullptr,
        DWORD  dwStyle    = WS_OVERLAPPEDWINDOW,
        DWORD  dwExStyle  = WS_EX_LEFT,
        int    x          = CW_USEDEFAULT,
        int    y          = CW_USEDEFAULT,
        HWND   hWndParent = nullptr,
        HMENU  hMenu      = nullptr
    ) noexcept
    {
        auto hInstance = GetModuleHandleW(nullptr);
        if (hInstance == nullptr) return false;

        WNDCLASSEXW wc   = {};
        wc.cbSize        = sizeof(wc);
        wc.hInstance     = hInstance;
        wc.lpszClassName = getClassName();
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = T::windowProc;

        if (RegisterClassExW(&wc) == 0) return false;

        if (width != CW_USEDEFAULT && height != CW_USEDEFAULT)
        {
            // Adjust real size, include title bar
            RECT rect = { 0, 0, width, height };
            if (AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false) == FALSE) return false;
            width  = rect.right - rect.left;
            height = rect.bottom - rect.top;
        }

        m_hWnd = CreateWindowExW(
            dwExStyle,
            getClassName(), windowName,
            dwStyle,
            //WS_POPUP, // If use popup(without title bar), you should also to delete code that adjust window real size
            x, y, width, height,
            hWndParent, hMenu, hInstance, this
        );
        if (m_hWnd == nullptr) return false;

        return true;
    }

    bool show() const noexcept
    {
        ShowWindow(m_hWnd, SW_SHOW);
        if (UpdateWindow(m_hWnd) == 0) return false;
        return true;
    }

    HWND getHWnd() const noexcept { return m_hWnd; }

protected:
    virtual PCWSTR  getClassName() const noexcept = 0;
    virtual LRESULT handleMessage(UINT msg, WPARAM wParam, LPARAM lParam) = 0;
    
    HWND m_hWnd = nullptr;
};