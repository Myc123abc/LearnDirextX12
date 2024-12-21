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
        else
            break;
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
    case WM_KEYUP:
        if (wParam == 'm' || wParam == 'M')
        {
            m_useMSAA = !m_useMSAA;
            createPipeline();
        }
        // if (wParam == 'w' || wParam == 'W')
        // {
        //     m_wireframe = !m_wireframe;
        //     createPipeline();
        // }
        return 0;

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
        onResize();
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
    
        ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
        ThrowIfFailed(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf()))); 
        dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
        dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

        DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
        {
            80  /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */,
        };
        DXGI_INFO_QUEUE_FILTER filter = {};
        filter.DenyList.NumIDs  = _countof(hide);
        filter.DenyList.pIDList = hide;
        dxgiInfoQueue->AddStorageFilterEntries(DXGI_DEBUG_DXGI, &filter);
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

    // Configure debug device
#if _DEBUG
    {
        ComPtr<ID3D12InfoQueue> infoQueue;
        ThrowIfFailed(m_device.As(&infoQueue));
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);

        D3D12_MESSAGE_ID hide[] =
        {
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
            // Workarounds for debug layer issuses on hybrid-graphics systems
            D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE,
            D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE,
        };
        D3D12_INFO_QUEUE_FILTER filter = {};
        filter.DenyList.NumIDs  = _countof(hide);
        filter.DenyList.pIDList = hide;
        infoQueue->AddStorageFilterEntries(&filter);
    }

#endif

    // ------------------------------------------------------
    //  Create command queue, allocator, list and swap chain
    // ------------------------------------------------------

    // Create command queue, allocator and list
    auto commandQueueDesc = D3D12_COMMAND_QUEUE_DESC();
    ThrowIfFailed(m_device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(m_commandQueue.GetAddressOf())));
    for (int i = 0; i < m_backBufferCount; ++i)
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

    // Create fence
    // CPU use signal to wait GPU has completed word so that CPU can right update resource
    ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.GetAddressOf())));

    // Create swap chain
    // Creating swap chain also creates back buffer resource, so there's not need to create back buffer resource manually.
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferDesc.Width  = m_width;
    swapChainDesc.BufferDesc.Height = m_height;
    swapChainDesc.BufferDesc.Format = m_backBufferFormat;
    swapChainDesc.SampleDesc.Count  = 1;
    swapChainDesc.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount       = m_backBufferCount;
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
    m_cbvSrvUavDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    
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

    for (int i = 0; i < m_backBufferCount; ++i)
        m_backbuffers[i]->SetName((std::wstring(L"Back Buffer ") + std::to_wstring(i)).c_str());


    //-------------
    // Handle MSAA
    //-------------

    // Get 4X MSAA quality level
    // All Direct3D 11 capable devices support 4X MSAA fir all render target formats
    // So we only need to check quality support
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS level = {};
    level.Format      = m_backBufferFormat;
    level.SampleCount = m_sampleCount;
    auto hr = m_device->CheckFeatureSupport(
        D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, 
        &level,
        sizeof(level)
    );
    if (FAILED(hr))
    {
        throw std::runtime_error("Not support 4xMSAA");
    }
    m_4xMSAAQualityLevels = level.NumQualityLevels - 1;

    if (m_useMSAA)
    {
        // Create descriptor Heap
        heapDesc = {};
        heapDesc.NumDescriptors = 1;
        heapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        ThrowIfFailed(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(m_MSAArtvHeap.GetAddressOf())));
        heapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        ThrowIfFailed(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(m_MSAAdsvHeap.GetAddressOf())));
        createMSAAResources();
    }

    // ------------------------------------
    //  Set viewport and scissor Rectangle
    // ------------------------------------

    m_viewport.Width     = static_cast<float>(m_width);
    m_viewport.Height    = static_cast<float>(m_height);
    m_viewport.MaxDepth  = 1.f;

    m_scissorRect.right  = m_width;
    m_scissorRect.bottom = m_height;
}

void DirectX12::createPipeline()
{
    if (m_shaders.empty() || m_inputLayout.empty())
        throw std::runtime_error("Set shaders and input layout before craeting pipeline!");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout    = { m_inputLayout.data(), static_cast<UINT>(m_inputLayout.size()) };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = 
    {
        reinterpret_cast<BYTE*>(m_shaders.at("vs")->GetBufferPointer()),
        m_shaders.at("vs")->GetBufferSize()
    };
    psoDesc.PS =
    {
        reinterpret_cast<BYTE*>(m_shaders.at("ps")->GetBufferPointer()),
        m_shaders.at("ps")->GetBufferSize()
    };
    psoDesc.RasterizerState       = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    // psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
    psoDesc.RasterizerState.AntialiasedLineEnable = TRUE;
    psoDesc.RasterizerState.MultisampleEnable = TRUE;
    psoDesc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState     = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask            = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets      = 1;
    psoDesc.RTVFormats[0]         = m_backBufferFormat;
    psoDesc.SampleDesc.Count      = m_useMSAA ? m_sampleCount : 1;
    psoDesc.SampleDesc.Quality    = m_useMSAA ? m_4xMSAAQualityLevels : 0;
    psoDesc.DSVFormat             = m_depthBufferFormat;
    // TODO: more flexbile which can contains different pso
    ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_psos["draw"].ReleaseAndGetAddressOf())));

    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_psos["wireframe"].ReleaseAndGetAddressOf())));
}

void DirectX12::createMSAAResources()
{
    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Width              = m_width;
    resourceDesc.Height             = m_height;
    resourceDesc.DepthOrArraySize   = 1;
    resourceDesc.MipLevels          = 1;
    resourceDesc.Format             = m_backBufferFormat;
    resourceDesc.SampleDesc.Count   = m_sampleCount;
    resourceDesc.SampleDesc.Quality = m_4xMSAAQualityLevels;
    resourceDesc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format   = m_backBufferFormat;
    clearValue.Color[0] = 40.f / 255;
    clearValue.Color[1] = 44.f / 255;
    clearValue.Color[2] = 52.f / 255;
    clearValue.Color[3] = 1.f;

    D3D12_HEAP_PROPERTIES heapProperties = {};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

    ThrowIfFailed(m_device->CreateCommittedResource(
        &heapProperties, 
        D3D12_HEAP_FLAG_NONE, 
        &resourceDesc,
        D3D12_RESOURCE_STATE_RENDER_TARGET, 
        &clearValue, 
        IID_PPV_ARGS(m_MSAArtv.ReleaseAndGetAddressOf())));
    m_MSAArtv->SetName(L"MSAA Render Target");

    resourceDesc.Format = m_depthBufferFormat;
    resourceDesc.Flags  = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    clearValue = {};
    clearValue.Format             = m_depthBufferFormat;
    clearValue.DepthStencil.Depth = 1.f;
    ThrowIfFailed(m_device->CreateCommittedResource(
        &heapProperties, 
        D3D12_HEAP_FLAG_NONE, 
        &resourceDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE, 
        &clearValue, 
        IID_PPV_ARGS(m_MSAAdsv.ReleaseAndGetAddressOf())));

    // Create descriptor
    m_device->CreateRenderTargetView(m_MSAArtv.Get(), nullptr, m_MSAArtvHeap->GetCPUDescriptorHandleForHeapStart());
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
    dsvDesc.Format        = m_depthBufferFormat;
    m_device->CreateDepthStencilView(m_MSAAdsv.Get(), &dsvDesc, m_MSAAdsvHeap->GetCPUDescriptorHandleForHeapStart());
}

void DirectX12::createRootSignature(CD3DX12_DESCRIPTOR_RANGE* pDescriptorRange, int num)
{
    std::vector<CD3DX12_ROOT_PARAMETER> rootParameter(num);
    for (int i = 0; i < num; ++i)
    {
        rootParameter[i].InitAsDescriptorTable(1, &pDescriptorRange[i]);
    }
    CD3DX12_ROOT_SIGNATURE_DESC desc(num, rootParameter.data(), 0, nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> serializedRootSignature;
    ComPtr<ID3DBlob> errorBlob;
    auto hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1,
        serializedRootSignature.GetAddressOf(), errorBlob.GetAddressOf());
    if (errorBlob != nullptr)
    {
        OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    }
    ThrowIfFailed(hr);

    ThrowIfFailed(m_device->CreateRootSignature(
        0, 
        serializedRootSignature->GetBufferPointer(),
        serializedRootSignature->GetBufferSize(),
        IID_PPV_ARGS(m_rootSignature.GetAddressOf())
    ));
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
                // TODO: don't use drawBegin and drawEnd
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
    if (m_wireframe)
    {
        ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_psos.at("wireframe").Get()));
    }
    else
    {
        ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_psos.at("draw").Get()));
    }

    // Reset viewport and scissor rectangle
    // These need to be reset when the command list is reset
    m_commandList->RSSetViewports(1, &m_viewport);

    // POINT center = { m_width / 2, m_height / 2 };
    // auto width = m_width / 2;
    // auto height = m_height / 2;
    // m_scissorRect.left = center.x - width / 2;
    // m_scissorRect.right = center.x + width / 2;
    // m_scissorRect.bottom = center.y + height / 2;
    // m_scissorRect.top = center.y - height / 2;
    m_commandList->RSSetScissorRects(1, &m_scissorRect);

    // Convert back buffer state from present to render target
    auto state = m_useMSAA ? D3D12_RESOURCE_STATE_RESOLVE_DEST : D3D12_RESOURCE_STATE_RENDER_TARGET;
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_backbuffers[m_backbufferIndex].Get(), 
        D3D12_RESOURCE_STATE_PRESENT, state);
    m_commandList->ResourceBarrier(1, &barrier);

    // Clear back buffers and depth buffer
    if (m_useMSAA)
    {
        static constexpr float color[] = { 40.f / 255, 44.f / 255, 52.f / 255, 1.f };
        m_commandList->ClearRenderTargetView(m_MSAArtvHeap->GetCPUDescriptorHandleForHeapStart(), color, 0, nullptr);
        m_commandList->ClearDepthStencilView(m_MSAAdsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);
    }
    else
    {
        static constexpr float color[] = { 40.f / 255, 44.f / 255, 52.f / 255, 1.f };
        auto backBufferDescriptorHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        backBufferDescriptorHandle.ptr += m_backbufferIndex * m_rtvDescriptorSize;
        m_commandList->ClearRenderTargetView(backBufferDescriptorHandle, color, 0, nullptr);
        m_commandList->ClearDepthStencilView(m_dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);
    }

    // Set render target
    if (m_useMSAA)
    {
        auto handle = m_MSAArtvHeap->GetCPUDescriptorHandleForHeapStart();
        auto handledep = m_MSAAdsvHeap->GetCPUDescriptorHandleForHeapStart();
        m_commandList->OMSetRenderTargets(1, &handle, true, &handledep);
    }
    else
    {
        auto backBufferDescriptorHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        backBufferDescriptorHandle.ptr += m_backbufferIndex * m_rtvDescriptorSize;
        auto depthBufferDescriptorHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
        m_commandList->OMSetRenderTargets(1, &backBufferDescriptorHandle, true, &depthBufferDescriptorHandle);
    }
}

void DirectX12::drawEnd()
{
    if (m_useMSAA)
    {
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_MSAArtv.Get(), 
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
        m_commandList->ResourceBarrier(1, &barrier);

        m_commandList->ResolveSubresource(
            m_backbuffers[m_backbufferIndex].Get(), 
            0, 
            m_MSAArtv.Get(), 
            0, 
            m_backBufferFormat);

        barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_backbuffers[m_backbufferIndex].Get(), 
            D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_PRESENT);
        m_commandList->ResourceBarrier(1, &barrier);

        barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_MSAArtv.Get(), 
            D3D12_RESOURCE_STATE_RESOLVE_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_commandList->ResourceBarrier(1, &barrier);
    }
    else
    {
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_backbuffers[m_backbufferIndex].Get(), 
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        m_commandList->ResourceBarrier(1, &barrier);
    }

    // Close commit list
    ThrowIfFailed(m_commandList->Close());
    // Add command list to queue
    m_commandQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList**>(m_commandList.GetAddressOf()));
    // Swap buffer
    ThrowIfFailed(m_swapChain->Present(0, 0));
    m_backbufferIndex = (m_backbufferIndex + 1) % m_backBufferCount;

    if (m_tmp_fr)
    {
        tmpFunc_drawEnd();
    }
    else 
    {
        flushCommandQueue();
    }
}

void DirectX12::onResize()
{
    // Wait GPU finish commands
    flushCommandQueue();

    // Reset command list
    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));


    if (m_useMSAA)
    {
        createMSAAResources();
    }

    // Reset back buffers and swap chain
    for (int i = 0; i < m_backBufferCount; ++i)
        m_backbuffers[i].Reset();

    ThrowIfFailed(m_swapChain->ResizeBuffers(
        m_backBufferCount, 
        m_width, m_height, 
        m_backBufferFormat, 
        DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH)
    );

    m_backbufferIndex = 0;

    auto rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (int i = 0; i < m_backBufferCount; ++i)
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
    ++m_currentFence;
    // Wait GPU execute complete
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_currentFence));
    // Wait until GPU has completed commands up to this fence point
    if (m_fence->GetCompletedValue() < m_currentFence)
    {
        HANDLE eventHandle = CreateEventExW(nullptr, nullptr, false, EVENT_ALL_ACCESS);
        // Fire event when GPU hits current fence
        ThrowIfFailed(m_fence->SetEventOnCompletion(m_currentFence, eventHandle));
        if (eventHandle == nullptr)
        {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }
        // Wait until GPU hits current fence event is fired
        WaitForSingleObjectEx(eventHandle, INFINITE, FALSE);
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

        m_phi = std::clamp(m_phi, 0.001f, std::numbers::pi_v<float> - 0.001f);
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