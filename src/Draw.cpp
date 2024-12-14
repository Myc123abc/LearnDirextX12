#include "Draw.hpp"
#include "Geometry.hpp"

using namespace DX;
using namespace DirectX;

Draw::Draw()
{
    m_tmp_fr = true;

    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));

    // Initialize root signature
    CD3DX12_DESCRIPTOR_RANGE cbvTable[2];
    cbvTable[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
    cbvTable[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);
    createRootSignature(cbvTable, 2);

    // Initialize shader and input layout
    setShader("vs", L"../shader/draw_vs.cso");
    setShader("ps", L"../shader/draw_ps.cso");
    m_inputLayout = 
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    buildGeometry();
    buildRenderItems();

    // Initialize frame resources
    for (auto& frameResource : m_frameResources)
    {
        frameResource = std::make_unique<FrameResource>(m_device.Get(), 1, m_renderItems.size());
    }
    m_currentFrameResource = m_frameResources[m_currentFrameResourceIndex].get();

    buildDescriptorHeaps();
    buildConstantBufferViews();
    createPipeline();

    ThrowIfFailed(m_commandList->Close());
    ID3D12CommandList* cmdLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
    flushCommandQueue();
}

void Draw::update()
{
    // keyboard input
    if (GetAsyncKeyState('w') & 0x8000 || GetAsyncKeyState('W') & 0x8000)
        m_wireframe = !m_wireframe;
    //TODO: MayBe i need createPipeline agian in here if

    // camera
    m_cameraPos.x = m_radius * sinf(m_phi) * cosf(m_theta);
    m_cameraPos.z = m_radius * sinf(m_phi) * sinf(m_theta);
    m_cameraPos.y = m_radius * cosf(m_phi);

    auto pos = XMVectorSet(m_cameraPos.x, m_cameraPos.y, m_cameraPos.z, 1.f);
    auto target = XMVectorZero();
    auto up = XMVectorSet(0.f, 1.f, 0.f, 0.f);
    
    auto view = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&m_view, view);

    // ----------------------------------
    //  Update to current frame resource
    // ----------------------------------

    m_currentFrameResourceIndex = (m_currentFrameResourceIndex + 1) % NumberOfFrameResources;
    m_currentFrameResource = m_frameResources[m_currentFrameResourceIndex].get();

    if (m_currentFrameResource->fence != 0 &&
        m_fence->GetCompletedValue() < m_currentFrameResource->fence)
    {
        HANDLE eventHandle = CreateEventExW(nullptr, nullptr, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(m_fence->SetEventOnCompletion(m_currentFrameResource->fence, eventHandle));
        WaitForSingleObjectEx(eventHandle, INFINITE, FALSE);
        CloseHandle(eventHandle);
    }

    // Update the object information like local coordinate system
    auto objectInfoUploadBuffer = m_currentFrameResource->objectInfo.get();
    for (auto& item : m_renderItems)
    {
        if (item->numFramesDirty > 0)
        {
            auto world = XMLoadFloat4x4(&item->world);
            
            ObjectInfo objectInfo;
            XMStoreFloat4x4(&objectInfo.world, XMMatrixTranspose(world));

            objectInfoUploadBuffer->copy(item->indexCount, objectInfo);

            --item->numFramesDirty;
        }
    }

    // Update the frame data
    view = XMLoadFloat4x4(&m_view);
    auto proj = XMLoadFloat4x4(&m_proj);

    auto viewProj = XMMatrixMultiply(view, proj);
    auto vec = XMMatrixDeterminant(view);
    auto invView = XMMatrixInverse(&vec, view);
    vec = XMMatrixDeterminant(proj);
    auto invProj = XMMatrixInverse(&vec, proj);
    vec = XMMatrixDeterminant(viewProj);
    auto invViewProj = XMMatrixInverse(&vec, viewProj);
    
    XMStoreFloat4x4(&m_frameData.view, XMMatrixTranspose(view));
    XMStoreFloat4x4(&m_frameData.invView, XMMatrixTranspose(invView));
    XMStoreFloat4x4(&m_frameData.proj, XMMatrixTranspose(proj));
    XMStoreFloat4x4(&m_frameData.invProj, XMMatrixTranspose(invProj));
    XMStoreFloat4x4(&m_frameData.viewProj, XMMatrixTranspose(viewProj));
    XMStoreFloat4x4(&m_frameData.invViewProj, XMMatrixTranspose(invViewProj));
    m_frameData.cameraPos = m_cameraPos;
    m_frameData.renderTargetSize = XMFLOAT2(m_width, m_height);
    m_frameData.invRenderTargetSize = XMFLOAT2(1.f / m_width, 1.f / m_height);
    m_frameData.nearZ = 1.f;
    m_frameData.farZ = 1000.f;
    m_frameData.totalTime = m_timer.getTime();
    m_frameData.deltaTime = m_timer.getDelta();

    auto frameDataUploadBuffer = m_currentFrameResource->frameData.get();
    frameDataUploadBuffer->copy(0, m_frameData);
}

void Draw::draw()
{
    auto cmdAlloc = m_currentFrameResource->cmdAlloc;
    
    ThrowIfFailed(cmdAlloc->Reset());

    if (m_wireframe)
        ThrowIfFailed(m_commandList->Reset(cmdAlloc.Get(), m_psos.at("wireframe").Get()));
    else
        ThrowIfFailed(m_commandList->Reset(cmdAlloc.Get(), m_psos.at("draw").Get()));
    
    m_commandList->RSSetViewports(1, &m_viewport);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);

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

    ID3D12DescriptorHeap* heaps[] = { m_cbvHeap.Get() };
    m_commandList->SetDescriptorHeaps(_countof(heaps), heaps);
    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    auto index = m_frameDataOffset + m_currentFrameResourceIndex;
    auto handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetGPUDescriptorHandleForHeapStart());
    handle.Offset(index, m_cbvSrvUavDescriptorSize);
    m_commandList->SetGraphicsRootDescriptorTable(1, handle);

    drawRenderItems(m_commandList.Get(), m_opaqueRenderItems);

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

void Draw::onResize() 
{
    DirectX12::onResize();
    auto p = XMMatrixPerspectiveFovLH(
        static_cast<float>(0.25f * std::numbers::pi), 
        static_cast<float>(m_width) / m_height,
        1.f,
        1000.f);
    XMStoreFloat4x4(&m_proj, p);
}

void Draw::tmpFunc_drawEnd()
{
    m_currentFrameResource->fence = ++m_currentFence;
    m_commandQueue->Signal(m_fence.Get(), m_currentFence);
}

void Draw::buildGeometry()
{
    auto box = Geometry::createBox(1.5f, 0.5f, 1.5f, 3);
    auto grid = Geometry::createGrid(20.f, 30.f, 60, 40);
    auto sphere = Geometry::createSphere(0.5f, 20, 20);
    auto cylinder = Geometry::createCylinder(0.5f, 0.3f, 3.0f, 20, 20);


    // Concatenating all the geometry into one big vertex/index buffer.

    size_t boxVertexOffset = 0;
    auto gridVertexOffset = box.vertices.size();
    auto sphereVertexOffset = gridVertexOffset + grid.vertices.size();
    auto cylinderVertexOffset = sphereVertexOffset + sphere.vertices.size();

    size_t boxIndexOffset = 0;
    auto gridIndexOffset = box.indices.size();
    auto sphereIndexOffset = gridIndexOffset + grid.indices.size();
    auto cylinderIndexOffset = sphereIndexOffset + sphere.indices.size();

    SubMeshGeometry boxSM{ box.indices.size(), boxIndexOffset, boxVertexOffset };
    SubMeshGeometry gridSM{ grid.indices.size(), gridIndexOffset, gridVertexOffset };
    SubMeshGeometry sphereSM{ sphere.indices.size(), sphereIndexOffset, sphereVertexOffset };
    SubMeshGeometry cylinderSM{ cylinder.indices.size(), cylinderIndexOffset, cylinderVertexOffset };

    std::vector<Vertex> vertices(
        box.vertices.size() + grid.vertices.size() +
        sphere.vertices.size() + cylinder.vertices.size());
    UINT k = 0;
    for (size_t i = 0; i < box.vertices.size(); ++i, ++k)
    {
        vertices[k].pos = box.vertices[i].pos;
        vertices[k].col = XMFLOAT4(Colors::DarkGreen);
    }
    for (size_t i = 0; i < grid.vertices.size(); ++i, ++k)
    {
        vertices[k].pos = grid.vertices[i].pos;
        vertices[k].col = XMFLOAT4(Colors::ForestGreen);
    }
    for (size_t i = 0; i < sphere.vertices.size(); ++i, ++k)
    {
        vertices[k].pos = sphere.vertices[i].pos;
        vertices[k].col = XMFLOAT4(Colors::Crimson);
    }
    for (size_t i = 0; i < cylinder.vertices.size(); ++i, ++k)
    {
        vertices[k].pos = cylinder.vertices[i].pos;
        vertices[k].col = XMFLOAT4(Colors::SteelBlue);
    }

    std::vector<uint16_t> indices;
    indices.insert(indices.end(), box.getIndices16().begin(), box.getIndices16().end());
    indices.insert(indices.end(), grid.getIndices16().begin(), grid.getIndices16().end());
    indices.insert(indices.end(), sphere.getIndices16().begin(), sphere.getIndices16().end());
    indices.insert(indices.end(), cylinder.getIndices16().begin(), cylinder.getIndices16().end());

    const uint32_t vSize = static_cast<uint32_t>(vertices.size()) * sizeof(Vertex);
    const uint32_t iSize = static_cast<uint32_t>(indices.size())  * sizeof(uint16_t);

    auto geo = std::make_unique<MeshGeometry>();
    geo->name = "Shapes";
    ThrowIfFailed(D3DCreateBlob(vSize, &geo->vertexBufferCPU));
    memcpy(geo->vertexBufferCPU->GetBufferPointer(), vertices.data(), vSize);
    ThrowIfFailed(D3DCreateBlob(iSize, &geo->indexBufferCPU));
    memcpy(geo->indexBufferCPU->GetBufferPointer(), indices.data(), iSize);

    geo->vertexBufferGPU = createDefaultBuffer(
        m_device.Get(),
        m_commandList.Get(), 
        vertices.data(), 
        vSize, 
        geo->vertexBufferUploader);
    geo->indexBufferGPU = createDefaultBuffer(
        m_device.Get(),
        m_commandList.Get(), 
        indices.data(), 
        iSize, 
        geo->indexBufferUploader);

    geo->vertexByteStride = sizeof(Vertex);
    geo->vertexBufferByteSize = vSize;
    geo->indexFormat = DXGI_FORMAT_R16_UINT;
    geo->indexBufferByteSize = iSize;

    geo->drawArgs["box"] = boxSM;
    geo->drawArgs["grid"] = gridSM;
    geo->drawArgs["sphere"] = sphereSM;
    geo->drawArgs["cylinder"] = cylinderSM;

    m_geometries[geo->name] = std::move(geo);
}

void Draw::buildRenderItems()
{
    auto box = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&box->world, XMMatrixScaling(2.f, 2.f, 2.f) * XMMatrixTranslation(0.f, 0.5f, 0.f));
    box->objCBIndex = 0;
    box->geo = m_geometries.at("Shapes").get();
    box->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    box->indexCount = box->geo->drawArgs.at("box").indexCount;
    box->startIndexLocation = box->geo->drawArgs.at("box").startIndexLocation;
    box->baseVertexLocation = box->geo->drawArgs.at("box").baseVertexLocation;
    m_renderItems.push_back(std::move(box));

    auto grid = std::make_unique<RenderItem>();
    grid->world = createIdentity4x4();
    grid->objCBIndex = 1;
    grid->geo = m_geometries.at("Shapes").get();
    grid->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    grid->indexCount = grid->geo->drawArgs.at("grid").indexCount;
    grid->startIndexLocation = grid->geo->drawArgs.at("grid").startIndexLocation;
    grid->baseVertexLocation = grid->geo->drawArgs.at("grid").baseVertexLocation;
    m_renderItems.push_back(std::move(grid));
    
    uint32_t index = 2;
    for (int i = 0; i < 5; ++i)
    {
        auto leftCylRitem = std::make_unique<RenderItem>();
        auto rightCylRitem = std::make_unique<RenderItem>();
        auto leftSphereRitem = std::make_unique<RenderItem>();
        auto rightSphereRitem = std::make_unique<RenderItem>();

        auto leftCylWorld = XMMatrixTranslation(-5.f, 1.5f, -10.f + i * 5.f);
        auto rightCylWorld = XMMatrixTranslation(+5.f, 1.5f, -10.f + i * 5.f);
        auto leftSphereWorld = XMMatrixTranslation(-5.f, 3.5f, -10.f + i * 5.f);
        auto rightSphereWorld = XMMatrixTranslation(+5.f, 3.5f, -10.f + i * 5.f);

        XMStoreFloat4x4(&leftCylRitem->world, leftCylWorld);
        leftCylRitem->objCBIndex = index++;
        leftCylRitem->geo = m_geometries.at("Shapes").get();
        leftCylRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        leftCylRitem->indexCount = leftCylRitem->geo->drawArgs.at("cylinder").indexCount;
        leftCylRitem->startIndexLocation = leftCylRitem->geo->drawArgs.at("cylinder").startIndexLocation;
        leftCylRitem->baseVertexLocation = leftCylRitem->geo->drawArgs.at("cylinder").baseVertexLocation;
        m_renderItems.push_back(std::move(leftCylRitem));

        XMStoreFloat4x4(&rightCylRitem->world, rightCylWorld);
        rightCylRitem->objCBIndex = index++;
        rightCylRitem->geo = m_geometries.at("Shapes").get();
        rightCylRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        rightCylRitem->indexCount = rightCylRitem->geo->drawArgs.at("cylinder").indexCount;
        rightCylRitem->startIndexLocation = rightCylRitem->geo->drawArgs.at("cylinder").startIndexLocation;
        rightCylRitem->baseVertexLocation = rightCylRitem->geo->drawArgs.at("cylinder").baseVertexLocation;
        m_renderItems.push_back(std::move(rightCylRitem));

        XMStoreFloat4x4(&leftSphereRitem->world, leftSphereWorld);
        leftSphereRitem->objCBIndex = index++;
        leftSphereRitem->geo = m_geometries.at("Shapes").get();
        leftSphereRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        leftSphereRitem->indexCount = leftSphereRitem->geo->drawArgs.at("sphere").indexCount;
        leftSphereRitem->startIndexLocation = leftSphereRitem->geo->drawArgs.at("sphere").startIndexLocation;
        leftSphereRitem->baseVertexLocation = leftSphereRitem->geo->drawArgs.at("sphere").baseVertexLocation;
        m_renderItems.push_back(std::move(leftSphereRitem));

        XMStoreFloat4x4(&rightSphereRitem->world, rightSphereWorld);
        rightSphereRitem->objCBIndex = index++;
        rightSphereRitem->geo = m_geometries.at("Shapes").get();
        rightSphereRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        rightSphereRitem->indexCount = rightSphereRitem->geo->drawArgs.at("sphere").indexCount;
        rightSphereRitem->startIndexLocation = rightSphereRitem->geo->drawArgs.at("sphere").startIndexLocation;
        rightSphereRitem->baseVertexLocation = rightSphereRitem->geo->drawArgs.at("sphere").baseVertexLocation;
        m_renderItems.push_back(std::move(rightSphereRitem));
    }

    for (auto& item : m_renderItems)
        m_opaqueRenderItems.push_back(item.get());
}

void Draw::buildDescriptorHeaps()
{
    auto size = static_cast<uint32_t>(m_opaqueRenderItems.size());
    auto numDescriptors = (size + 1) * NumberOfFrameResources;
    m_frameDataOffset = size * NumberOfFrameResources;

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = numDescriptors;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(m_device->CreateDescriptorHeap(
        &desc, 
        IID_PPV_ARGS(m_cbvHeap.GetAddressOf())));
}

void Draw::buildConstantBufferViews()
{
    uint32_t byteSize = getMultiplesOf256<sizeof(ObjectInfo)>();
    uint32_t objCount = m_opaqueRenderItems.size();

    for (int frameIndex = 0; frameIndex < NumberOfFrameResources; ++frameIndex)
    {
        auto buffer = m_frameResources[frameIndex]->frameData->get();
        for (uint32_t i = 0; i < objCount; ++i)
        {
            auto addr = buffer->GetGPUVirtualAddress();
            addr += i * byteSize;
            int heapIndex = frameIndex * objCount + i;
            auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
            handle.Offset(heapIndex, m_cbvSrvUavDescriptorSize);

            D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
            desc.BufferLocation = addr;
            desc.SizeInBytes = byteSize;
            m_device->CreateConstantBufferView(&desc, handle);
        }
    }

    byteSize = getMultiplesOf256<sizeof(FrameData)>();

    for (int frameIndex = 0; frameIndex < NumberOfFrameResources; ++frameIndex)
    {
        auto buffer = m_frameResources[frameIndex]->frameData->get();
        auto addr = buffer->GetGPUVirtualAddress();
        int heapIndex = m_frameDataOffset + frameIndex;
        auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
        handle.Offset(heapIndex, m_cbvSrvUavDescriptorSize);

        D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
        desc.BufferLocation = addr;
        desc.SizeInBytes = byteSize;
        m_device->CreateConstantBufferView(&desc, handle);
    }
}

void Draw::drawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& renderItems)
{
    auto size = getMultiplesOf256<sizeof(ObjectInfo)>();
    auto objInfo = m_currentFrameResource->objectInfo.get();
    for (auto* ri : renderItems)
    {
        auto vView = ri->geo->VertexBufferView();
        cmdList->IASetVertexBuffers(0, 1, &vView);
        auto iView= ri->geo->IndexBufferView();
        cmdList->IASetIndexBuffer(&iView);
        cmdList->IASetPrimitiveTopology(ri->primitiveType);

        auto index = m_currentFrameResourceIndex * m_opaqueRenderItems.size() + ri->objCBIndex;
        auto handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetGPUDescriptorHandleForHeapStart());
        handle.Offset(index, m_cbvSrvUavDescriptorSize);

        cmdList->SetGraphicsRootDescriptorTable(0, handle);
        cmdList->DrawIndexedInstanced(ri->indexCount, 1, ri->startIndexLocation, ri->baseVertexLocation, 0);
    }
}