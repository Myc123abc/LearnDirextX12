#pragma once

#include "Timer.hpp"

class DirectX12
{
public:
    DirectX12();
    ~DirectX12() = default;

    DirectX12(const DirectX12&)            = delete;
    DirectX12(DirectX12&&)                 = delete;
    DirectX12& operator=(const DirectX12&) = delete;
    DirectX12& operator=(DirectX12&&)      = delete;

    static DirectX12* get() noexcept
    {
        return s_pThis;
    }

    void run();

    LRESULT CALLBACK wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
    void flushCommandQueue();

    virtual void onResize();
    virtual void update() {}
    virtual void draw()   {}

    virtual void onMouseDown(WPARAM btnState, int x, int y);
    virtual void onMouseUp(WPARAM btnState, int x, int y);
    virtual void onMouseMove(WPARAM btnState, int x, int y);
    virtual void onMouseWheel(WPARAM wParam);

    inline void setShader(std::string_view name, std::wstring_view path) { m_shaders[name.data()] = DX::loadBinaryFile(path); }
    // Must set shaders, and input layout before creating pipeline
    void createPipeline();

    void createMSAAResources();

    void createRootSignature(CD3DX12_DESCRIPTOR_RANGE* descriptorRanges, int num);

private:
    void drawBegin();
    void drawEnd();

protected:
    inline static DirectX12* s_pThis = nullptr; // For singleton

    HWND m_hWnd;
    int  m_width     = 800;
    int  m_height    = 600;
    bool m_paused    = false;
    bool m_minimized = false;
    bool m_maximized = false;
    bool m_resizing  = false;

    GalgameEngine::Timer m_timer;

    static constexpr int MaxBackBufferCount = 3;
    static constexpr int m_backBufferCount = 2; 

    Microsoft::WRL::ComPtr<IDXGIFactory4> m_factory;    // Use for hardware and display management
                                                        // such as enumerating available GPUs, creating swap chains, managing display-related events
    Microsoft::WRL::ComPtr<ID3D12Device>  m_device;     // Use for interacts with GPU
                                                        // such as resource creation, rendering, and command execution

    DXGI_FORMAT m_backBufferFormat  = DXGI_FORMAT_R8G8B8A8_UNORM;    // 32-bit color format, unsigned format (0.0 ~ 1.0 <=> 0 ~ 255)
    DXGI_FORMAT m_depthBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    bool        m_wireframe = false;
    bool        m_useMSAA = true;
    int         m_4xMSAAQualityLevels;
    int         m_sampleCount = 4;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_MSAArtvHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_MSAAdsvHeap;
    Microsoft::WRL::ComPtr<ID3D12Resource>       m_MSAArtv;
    Microsoft::WRL::ComPtr<ID3D12Resource>       m_MSAAdsv;

    Microsoft::WRL::ComPtr<ID3D12CommandQueue>        m_commandQueue;     // Submit command lists to GPU to execute
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>    m_commandAllocator; // Allocate memory for commands
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;      // Records commands

    Microsoft::WRL::ComPtr<ID3D12Fence>   m_fence;      // Use for CPU GPU synchronization
    UINT64                                m_currentFence = 0;

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
    UINT                                         m_cbvSrvUavDescriptorSize;

    /*
    * Resource is actual resource memory for GPU
    * CPU need to use descriptor bulid relationship with resource to access them
    */
    Microsoft::WRL::ComPtr<ID3D12Resource> m_backbuffers[m_backBufferCount];
    UINT                                   m_backbufferIndex = 0;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_depthBuffer;

    D3D12_VIEWPORT m_viewport    = {};
    D3D12_RECT     m_scissorRect = {};



    std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> m_psos;

    POINT m_mousePosition = {};

    // Camera
    float m_theta  = 1.5f * DirectX::XM_PI;
    float m_phi    = DirectX::XM_PIDIV2; // DirectX::XM_PIDIV4;
    float m_radius = 5.f;

    // Coordinate System
    DirectX::XMFLOAT4X4 m_world = DX::createIdentity4x4();
    DirectX::XMFLOAT4X4 m_view  = DX::createIdentity4x4();
    DirectX::XMFLOAT4X4 m_proj  = DX::createIdentity4x4();

    Microsoft::WRL::ComPtr<ID3D12RootSignature>  m_rootSignature;
    std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> m_shaders;
    std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;


    // TODO: use frame resources way not flushCommandQueue
    bool m_tmp_fr = false;
    virtual void tmpFunc_drawEnd() {}
};