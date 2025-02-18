#pragma once

#include "Timer.hpp"

#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_4.h>

class DirectX12
{
public:
    DirectX12(int width, int height);
    ~DirectX12() {
        Microsoft::WRL::ComPtr<ID3D12DebugDevice> debugDevice;
if (SUCCEEDED(m_device->QueryInterface(IID_PPV_ARGS(&debugDevice))))
{
    debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
}

    }
    
    DirectX12(const DirectX12&)            = delete;
    DirectX12(DirectX12&&)                 = delete;
    DirectX12& operator=(const DirectX12&) = delete;
    DirectX12& operator=(DirectX12&&)      = delete;

    static DirectX12* get() noexcept
    {
        return s_pThis;
    }

    void run();

    void render();

    void onResize();

    LRESULT CALLBACK wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    void flushCommandQueue();

private:
    inline static DirectX12* s_pThis = nullptr; // For singleton

    HWND m_hWnd;
    int  m_width, m_height;
    bool m_paused    = false;
    bool m_minimized = false;
    bool m_maximized = false;
    bool m_resizing  = false;

    GalgameEngine::Timer m_timer;

    Microsoft::WRL::ComPtr<IDXGIFactory4> m_factory;    // Use for hardware and display management
                                                        // such as enumerating available GPUs, creating swap chains, managing display-related events
    Microsoft::WRL::ComPtr<ID3D12Device>  m_device;     // Use for interacts with GPU
                                                        // such as resource creation, rendering, and command execution

    Microsoft::WRL::ComPtr<ID3D12Fence>   m_fence;      // Use for CPU GPU synchronization

    DXGI_FORMAT m_backBufferFormat  = DXGI_FORMAT_R8G8B8A8_UNORM;    // 32-bit color format, unsigned format (0.0 ~ 1.0 <=> 0 ~ 255)
    DXGI_FORMAT m_depthBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    UINT        m_4xMSAAQualityLevels;

    Microsoft::WRL::ComPtr<ID3D12CommandQueue>        m_commandQueue;     // Submit command lists to GPU to execute
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>    m_commandAllocator; // Allocate memory for commands
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;      // Records commands

    Microsoft::WRL::ComPtr<IDXGISwapChain> m_swapChain;

    /*
    * Descriptor is a structure that stores resource information
    * Descriptor Heap is an array of descriptors
    * Descriptor Handle is a pointer to a descriptor in Descriptor Heap
    * Descriptor Size is used to offset descriptor in its heap
    */
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    UINT                                         m_rtvDescriptorSize;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
    UINT                                         m_dsvDescriptorSize;

    /*
    * Resource is actual resource memory for GPU
    * CPU need to use descriptor bulid relationship with resource to access them
    */
    Microsoft::WRL::ComPtr<ID3D12Resource> m_backbuffers[2];
    UINT                                   m_currentBackbufferIndex = 0;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_depthBuffer;

    D3D12_VIEWPORT m_viewport    = {};
    D3D12_RECT     m_scissorRect = {};
};