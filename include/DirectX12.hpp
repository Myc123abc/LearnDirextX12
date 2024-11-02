#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_4.h>

class DirectX12 final
{
public:
    DirectX12(HWND hWnd, int width, int height);
    ~DirectX12() = default;
    
    DirectX12(const DirectX12&)            = delete;
    DirectX12(DirectX12&&)                 = delete;
    DirectX12& operator=(const DirectX12&) = delete;
    DirectX12& operator=(DirectX12&&)      = delete;

    void render();

private:
    int m_width, m_height;

    Microsoft::WRL::ComPtr<IDXGIFactory4> m_factory;
};