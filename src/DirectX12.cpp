#include "DirectX12.hpp"
#include "WindowsUtil.hpp"

#include <string>
#include <format>
#include <numbers>

#include <assert.h>

using namespace Microsoft::WRL;
using namespace Win;
using namespace DX;


LRESULT CALLBACK wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    
    case WM_KEYUP:
        if (wParam == VK_ESCAPE)
            PostQuitMessage(0);
        return 0;
    
    case WM_GETMINMAXINFO:
        reinterpret_cast<MINMAXINFO*>(lParam)->ptMinTrackSize.x = 200;
        reinterpret_cast<MINMAXINFO*>(lParam)->ptMinTrackSize.y = 200;
        return 0;

    case WM_MENUCHAR:
        // Disable beep when Alt + Enter
        return MAKELRESULT(0, MNC_CLOSE);
    }
    return DirectX12::get()->wndProc(hWnd, msg, wParam, lParam);
}

LRESULT DirectX12::wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{ 
    switch (msg)
    {
    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE)
        {
            m_paused = true;
            m_timer.pause();
        }
        else
        {
            m_paused = false;
            m_timer.resume();
        }
        return 0;
    
    case WM_SIZE:
        m_width  = LOWORD(lParam);
        m_height = HIWORD(lParam);
        if (m_device)
        {
            if (wParam == SIZE_MINIMIZED)
            {
                m_minimized = true;
                m_maximized = false;
            }
            else if (wParam == SIZE_MAXIMIZED)
            {
                m_minimized = false;
                m_maximized = true;
                onResize();
            }
            else if (wParam == SIZE_RESTORED)
            {
                if (m_minimized)
                {
                    m_minimized = false;
                    onResize();
                }
                else if (m_maximized)
                {
                    m_maximized = false;
                    onResize();
                }
                else if (m_resizing)
                {

                }
                else
                {
                    onResize();
                }
            }
        }
        return 0;

    case WM_ENTERSIZEMOVE:
        m_paused   = true;
        m_resizing = true;
        m_timer.pause();
        return 0;

    case WM_EXITSIZEMOVE:
        m_paused   = false;
        m_resizing = false;
        m_timer.resume();
        return 0;

    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
        onMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;

    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
        onMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;

    case WM_MOUSEMOVE:
        onMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;

    case WM_MOUSEWHEEL:
        onMouseWheel(wParam);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

DirectX12::DirectX12()
{
   // Singleton
    assert(s_pThis == nullptr);
    s_pThis = this;

    enableMemCheck();

    ThrowIfFalse(DirectX::XMVerifyCPUSupport());

    // ---------------
    //  Create window
    // ---------------
    WindowConfig config;
    config.className = L"LearnDirectX12";
    config.width     = m_width;
    config.height    = m_height;
    config.wndProc   = ::wndProc;
    m_hWnd = createWindow(config);
    ThrowIfFalse(m_hWnd);

    // --------------------------
    //  Enable debug information
    // --------------------------

#ifdef _DEBUG
    {
        // Enable D3D12 debug layer
        ComPtr<ID3D12Debug> debugController;
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf())));
        debugController->EnableDebugLayer();
    }

    // ----------------------------------
    //  Create factory, device and fence
    // ----------------------------------

    // Create factory
    // Enable debug feature of factory
    ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(m_factory.GetAddressOf())));
#else
    ThrowIfFailed(CreateDXGIFactory(IID_PPV_ARGS(m_factory.GetAddressOf())));
#endif

    // Create device
    if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(m_device.GetAddressOf()))))
    {
        // Fallback to WARP device
        ComPtr<IDXGIAdapter> warpAdapter;
        ThrowIfFailed(m_factory->EnumWarpAdapter(IID_PPV_ARGS(warpAdapter.GetAddressOf())));
        ThrowIfFailed(D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(m_device.GetAddressOf())));
    }

    // Create fence
    // CPU use signal to wait GPU has completed word so that CPU can right update resource
    ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.GetAddressOf())));

    // ------------------------------------------------------
    //  Create command queue, allocator, list and swap chain
    // ------------------------------------------------------

    // Get 4X MSAA quality level
    // All Direct3D 11 capable devices support 4X MSAA fir all render target formats
    // So we only need to check quality support
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS level = {};
    level.Format      = m_backBufferFormat;
    level.SampleCount = 4;
    ThrowIfFailed(m_device->CheckFeatureSupport(
        D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, 
        &level,
        sizeof(level)
    ));
    m_4xMSAAQualityLevels = level.NumQualityLevels;

    // Create command queue, allocator and list
    auto commandQueueDesc = D3D12_COMMAND_QUEUE_DESC();
    ThrowIfFailed(m_device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(m_commandQueue.GetAddressOf())));
    ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_commandAllocator.GetAddressOf())));
    ThrowIfFailed(m_device->CreateCommandList(
        0, 
        D3D12_COMMAND_LIST_TYPE_DIRECT, 
        m_commandAllocator.Get(),
        nullptr,
        IID_PPV_ARGS(m_commandList.GetAddressOf())
    ));
    // Close command list before reset
    // We always need reset the command list before rendering new frame
    m_commandList->Close();

    // Create swap chain
    // Creating swap chain also creates back buffer resource, so there's not need to create back buffer resource manually.
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferDesc.Width  = m_width;
    swapChainDesc.BufferDesc.Height = m_height;
    swapChainDesc.BufferDesc.Format = m_backBufferFormat;
    swapChainDesc.SampleDesc.Count  = 1;
    swapChainDesc.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount       = 2;
    swapChainDesc.OutputWindow      = m_hWnd;
    swapChainDesc.Windowed          = true;
    swapChainDesc.SwapEffect        = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Flags             = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    ThrowIfFailed(m_factory->CreateSwapChain(
        m_commandQueue.Get(), 
        &swapChainDesc, 
        m_swapChain.GetAddressOf()
    ));

    // Disable Alt + Enter to fullscreen, it will lead ComPtr release error
    ThrowIfFailed(m_factory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER));

    // ---------------------------------------
    //  Get descriptor size
    //  Create descriptor heap and descriptor
    // ---------------------------------------

    // Get rtv and dsv descriptor size
    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    
    // Create descriptor heaps for back buffer and depth buffer
    // CPU will use them to access GPU resource
    // Notice, there is only create the descriptor heap, not create the descriptor
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 2;
    heapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(m_rtvHeap.GetAddressOf())));
    heapDesc.NumDescriptors = 1;
    heapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(m_dsvHeap.GetAddressOf())));

    // Create back buffer descriptor
    // Get the first back buffer descriptor handle
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    // Get the pointer of the first back buffer resource
    ThrowIfFailed(m_swapChain->GetBuffer(0, IID_PPV_ARGS((m_backbuffers[0].GetAddressOf()))));
    // Create the first buffer descriptor on the first buffer resource
    m_device->CreateRenderTargetView(m_backbuffers[0].Get(), nullptr, rtvHandle);
    // Move to second back buffer do the same operation as before
    rtvHandle.ptr += m_rtvDescriptorSize;
    ThrowIfFailed(m_swapChain->GetBuffer(1, IID_PPV_ARGS(m_backbuffers[1].GetAddressOf())));
    m_device->CreateRenderTargetView(m_backbuffers[1].Get(), nullptr, rtvHandle);

    // Create depth buffer resource and descriptor
    // Create depth buffer resource
    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension           = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Width               = m_width;
    resourceDesc.Height              = m_height;
    resourceDesc.DepthOrArraySize    = 1;
    resourceDesc.MipLevels           = 1;
    resourceDesc.Format              = m_depthBufferFormat;
    resourceDesc.SampleDesc.Count    = 1;
    resourceDesc.Flags               = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clearValue  = {};
    clearValue.Format             = m_depthBufferFormat;
    clearValue.DepthStencil.Depth = 1.f;

    D3D12_HEAP_PROPERTIES heapProperties = {};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

    ThrowIfFailed(m_device->CreateCommittedResource(
        &heapProperties, 
        D3D12_HEAP_FLAG_NONE, 
        &resourceDesc, 
        D3D12_RESOURCE_STATE_DEPTH_WRITE, 
        &clearValue, 
        IID_PPV_ARGS(m_depthBuffer.GetAddressOf())
    ));
    
    // Create depth buffer descriptor
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Format        = m_depthBufferFormat;
    m_device->CreateDepthStencilView(m_depthBuffer.Get(), &dsvDesc, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

    // ------------------------------------
    //  Set viewport and scissor Rectangle
    // ------------------------------------

    m_viewport.Width     = static_cast<float>(m_width);
    m_viewport.Height    = static_cast<float>(m_height);
    m_viewport.MaxDepth  = 1.f;

    m_scissorRect.right  = m_width;
    m_scissorRect.bottom = m_height;
}

void DirectX12::run()
{
    MSG msg = {};

    m_timer.setFunc([this] {
        SetWindowTextA(this->m_hWnd, std::format("time:{} fps:{} mspf:{:.2f}", (int)m_timer.getTime(), (int)m_timer.getFPS(), m_timer.getMSPF()).c_str());
    });
    m_timer.reset();

    // Initialize DirectX12 resources finished, show window
    showWindow(m_hWnd);

    while (msg.message != WM_QUIT)
    {
        if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        else
        {
            m_timer.update();
            m_timer.calculateFrameState();

            if (!m_paused)
            {
                update();
                drawBegin();
                draw();
                drawEnd();
            }
            else
            {
                Sleep(100);
            }
        }
    }
}

void DirectX12::drawBegin()
{
    // Reset command list and allocator
    ThrowIfFailed(m_commandAllocator->Reset());
    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pso.Get()));

    // Reset viewport and scissor rectangle
    // These need to be reset when the command list is reset
    m_commandList->RSSetViewports(1, &m_viewport);

    POINT center = { m_width / 2, m_height / 2 };
    auto width = m_width / 2;
    auto height = m_height / 2;
    m_scissorRect.left = center.x - width / 2;
    m_scissorRect.right = center.x + width / 2;
    m_scissorRect.bottom = center.y + height / 2;
    m_scissorRect.top = center.y - height / 2;
    m_commandList->RSSetScissorRects(1, &m_scissorRect);

    // Convert back buffer state from present to render target
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_backbuffers[m_currentBackbufferIndex].Get(), 
        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_commandList->ResourceBarrier(1, &barrier);

    // Clear back buffers and depth buffer
    auto backBufferDescriptorHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    backBufferDescriptorHandle.ptr += m_currentBackbufferIndex * m_rtvDescriptorSize;
    auto depthBufferDescriptorHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
    // Clear color
    static constexpr float color[] = { 40.f / 255, 44.f / 255, 52.f / 255, 1.f };
    m_commandList->ClearRenderTargetView(backBufferDescriptorHandle, color, 0, nullptr);
    m_commandList->ClearDepthStencilView(depthBufferDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);

    // Set render target
    m_commandList->OMSetRenderTargets(1, &backBufferDescriptorHandle, true, &depthBufferDescriptorHandle);
}

void DirectX12::drawEnd()
{
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_backbuffers[m_currentBackbufferIndex].Get(), 
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    m_commandList->ResourceBarrier(1, &barrier);

    // Close commit list
    ThrowIfFailed(m_commandList->Close());
    // Add command list to queue
    m_commandQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList**>(m_commandList.GetAddressOf()));
    // Swap buffer
    ThrowIfFailed(m_swapChain->Present(0, 0));
    m_currentBackbufferIndex = (m_currentBackbufferIndex + 1) % 2;

    flushCommandQueue();
}

void DirectX12::onResize()
{
    // Wait GPU finish commands
    flushCommandQueue();

    // Reset command list
    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));

    // Reset back buffers and swap chain
    m_backbuffers[0].Reset();
    m_backbuffers[1].Reset();

    ThrowIfFailed(m_swapChain->ResizeBuffers(
        2, 
        m_width, m_height, 
        m_backBufferFormat, 
        DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH)
    );

    m_currentBackbufferIndex = 0;

    auto rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (int i = 0; i < 2; ++i)
    {
        ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(m_backbuffers[i].GetAddressOf())));
        m_device->CreateRenderTargetView(m_backbuffers[i].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += m_rtvDescriptorSize;
    }

    // Reset depth buffer and descriptor
    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension           = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Width               = m_width;
    resourceDesc.Height              = m_height;
    resourceDesc.DepthOrArraySize    = 1;
    resourceDesc.MipLevels           = 1;
    resourceDesc.Format              = DXGI_FORMAT_R24G8_TYPELESS;
    resourceDesc.SampleDesc.Count    = 1;
    resourceDesc.Flags               = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clearVal  = {};
    clearVal.Format             = m_depthBufferFormat;
    clearVal.DepthStencil.Depth = 1.f;

    D3D12_HEAP_PROPERTIES heapProperties = {};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

    ThrowIfFailed(m_device->CreateCommittedResource(
        &heapProperties, 
        D3D12_HEAP_FLAG_NONE, 
        &resourceDesc, 
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearVal, 
        IID_PPV_ARGS(m_depthBuffer.GetAddressOf())
    ));

    D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
    desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    desc.Format        = m_depthBufferFormat;
    m_device->CreateDepthStencilView(m_depthBuffer.Get(), &desc, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

    // Execute commands
    ThrowIfFailed(m_commandList->Close());
    m_commandQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList**>(m_commandList.GetAddressOf()));

    flushCommandQueue();

    // Reset viewport and scissor rectangle
    m_viewport.Width  = static_cast<float>(m_width);
    m_viewport.Height = static_cast<float>(m_height);

    m_scissorRect = { 0, 0, m_width, m_height};
}

void DirectX12::flushCommandQueue()
{
    // Wait GPU execute complete
    static UINT64 fence;
    ++fence;
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
    // Wait until GPU has completed commands up to this fence point
    if (m_fence->GetCompletedValue() < fence)
    {
        HANDLE eventHandle = CreateEventExW(nullptr, nullptr, false, EVENT_ALL_ACCESS);
        // Fire event when GPU hits current fence
        ThrowIfFailed(m_fence->SetEventOnCompletion(fence, eventHandle));
        if (eventHandle == nullptr)
        {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }
        // Wait until GPU hits current fence event is fired
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }
}

void DirectX12::onMouseDown(WPARAM btnState, int x, int y)
{
    m_mousePosition.x = x;
    m_mousePosition.y = y;
    SetCapture(m_hWnd);
}

void DirectX12::onMouseUp(WPARAM btnState, int x, int y) 
{
    ReleaseCapture();
}

void DirectX12::onMouseMove(WPARAM btnState, int x, int y) 
{
    if (btnState & MK_LBUTTON)
    {
        float dx = DirectX::XMConvertToRadians(0.25f * static_cast<float>(x - m_mousePosition.x));
        float dy = DirectX::XMConvertToRadians(0.25f * static_cast<float>(y - m_mousePosition.y));

        m_theta -= dx;
        m_phi   -= dy;

        m_phi = std::clamp(m_phi, 0.1f, std::numbers::pi_v<float> - 0.1f);
    }
    else if (btnState & MK_RBUTTON)
    {
        float dx = 0.005f * static_cast<float>(x - m_mousePosition.x);
        float dy = 0.005f * static_cast<float>(y - m_mousePosition.y);

        m_radius -= dx - dy;
        m_radius = std::clamp(m_radius, 3.f, 15.f);
    }

    m_mousePosition.x = x;
    m_mousePosition.y = y;
}

void DirectX12::onMouseWheel(WPARAM wParam)
{
    auto delta = GET_WHEEL_DELTA_WPARAM(wParam);

    constexpr float zoomSensitivity = 0.005f;

    m_radius -= delta * zoomSensitivity;

    m_radius = std::clamp(m_radius, 3.f, 15.f);
}