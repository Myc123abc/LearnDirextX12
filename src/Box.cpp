#include "Box.hpp"

#include <DirectXColors.h>

#include <array>
#include <numbers>

using namespace Microsoft::WRL;
using namespace DirectX::Colors;
using namespace DirectX;
using namespace PackedVector;
using namespace DX;

Box::Box()
{

    // -----------------------
    //  Create root signature
    // -----------------------

    CD3DX12_DESCRIPTOR_RANGE descriptorRange[1];
    descriptorRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
    createRootSignature(descriptorRange, 1);


    // ----------------------------------
    //  Create constant buffer resources
    // ----------------------------------

    // Hardware requires multiples of 256 for constant data
    int size = getMultiplesOf256<sizeof(ObjectConstant)>();
    
    D3D12_HEAP_PROPERTIES heapProp = {};
    heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.SampleDesc.Count    = 1;
    resourceDesc.SampleDesc.Quality  = 0;
    resourceDesc.DepthOrArraySize    = 1;
    resourceDesc.Width               = size;
    resourceDesc.Height              = 1;
    resourceDesc.Layout              = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.MipLevels           = 1;

    ThrowIfFailed(m_device->CreateCommittedResource(
        &heapProp,
        D3D12_HEAP_FLAG_NONE, 
        &resourceDesc, 
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, 
        IID_PPV_ARGS(m_constBuffer.GetAddressOf())
    ));

    ThrowIfFailed(m_constBuffer->Map(0, nullptr, &m_mappedData));

    // Create constant buffer descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 1;
    heapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(m_cbvHeap.GetAddressOf())));

    // Create constant buffer descriptor
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = m_constBuffer->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes    = size;
    m_device->CreateConstantBufferView(&cbvDesc, m_cbvHeap->GetCPUDescriptorHandleForHeapStart());


    // --------
    //  Layout
    // --------

    setShader("vs", L"../shader/box_vs.cso");
    setShader("ps", L"../shader/box_ps.cso");

    m_inputLayout =
    {
        {"COLOR",    0, DXGI_FORMAT_B8G8R8A8_UNORM, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        // {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        // {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    createPipeline();
#pragma region vertices and indices
    // ----------------------
    //  Vertices and Indices
    // ----------------------

    // std::vector<Vertex> vertices = 
    // {
    //     {{ 0.0f,  0.5f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}}, // Top
    //     {{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}}, // Right
    //     {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}}, // Left
    //     {{ 0.5f, -1.0f, 0.0f}, {1.0f, 1.0f, 0.0f, 1.0f}}  // Bottom
    // };
    

    std::array<VPosData, 14> verticesPos =
    {
        // Box
	    // VPosData(XMFLOAT3(.0f, .5f, .0f)),
	    // VPosData(XMFLOAT3(.5f, -0.5f, .0f)),
	    // VPosData(XMFLOAT3(-.5f, -0.5f, .0f)),
	    // VPosData(XMFLOAT3(.5f, 1.f, .0f)),

        // VPosData(XMFLOAT3(-0.0f, +1.0f, +0.0f)),
        // VPosData(XMFLOAT3(-1.0f, +0.0f, -1.0f)),
        // VPosData(XMFLOAT3(+1.0f, +0.0f, -1.0f)),
        // VPosData(XMFLOAT3(+1.0f, +0.0f, +1.0f)),
        // VPosData(XMFLOAT3(-1.0f, +0.0f, +1.0f)),

        // VPosData(XMFLOAT3(-1.0f, +0.0f, -1.0f)),

        VPosData(XMFLOAT3(-1.0f, -1.0f, -1.0f)),
	    VPosData(XMFLOAT3(-1.0f, +1.0f, -1.0f)),
	    VPosData(XMFLOAT3(+1.0f, +1.0f, -1.0f)),
	    VPosData(XMFLOAT3(+1.0f, -1.0f, -1.0f)),
	    VPosData(XMFLOAT3(-1.0f, -1.0f, +1.0f)),
	    VPosData(XMFLOAT3(-1.0f, +1.0f, +1.0f)),
	    VPosData(XMFLOAT3(+1.0f, +1.0f, +1.0f)),
	    VPosData(XMFLOAT3(+1.0f, -1.0f, +1.0f)),

        // Point 
        VPosData(XMFLOAT3(-0.5f, +0.0f, -1.1f)),
        VPosData(XMFLOAT3(+0.0f, +0.7f, -1.1f)),
        VPosData(XMFLOAT3(+0.5f, +0.8f, -1.1f)),
        VPosData(XMFLOAT3(+0.5f, +1.2f, -0.5f)),
        VPosData(XMFLOAT3(+0.f, +.5f, -0.0f)),

        // Pyramid
        VPosData(XMFLOAT3(0.f, 1.f, 0.f))
    };

    

    // Move cube left 2 units
    // auto moveleft2 = XMMatrixTranslation(-2.f, 0.f, 0.f);
    // for (int i = 0; i < 8; ++i)
    // {
    //     auto pos = XMLoadFloat3(&verticesPos[i].pos);
    //     pos = XMVector3Transform(pos, moveleft2);
    //     XMStoreFloat3(&verticesPos[i].pos, pos);
    // }


    const std::array<VColorData, 17> verticesColor =
    {
        VColorData(XMCOLOR(0, 0, 0, 255)),
        VColorData(XMCOLOR(255, 0, 0, 255)),
        VColorData(XMCOLOR(0, 255, 0, 255)),
        VColorData(XMCOLOR(0, 0, 255, 255)),
        VColorData(XMCOLOR(255, 255, 0, 255)),
        VColorData(XMCOLOR(255, 0, 255, 255)),
        VColorData(XMCOLOR(0, 255, 255, 255)),
        VColorData(XMCOLOR(255, 255, 255, 255)),
		// VColorData(XMFLOAT4(Colors::Red)),
		// VColorData(XMFLOAT4(Colors::Green)),
		// VColorData(XMFLOAT4(Colors::Blue)),
		// VColorData(XMFLOAT4(Colors::Yellow)),
		// VColorData(XMFLOAT4(Colors::Green)),
		// VColorData(XMFLOAT4(Colors::Cyan)),
		// VColorData(XMFLOAT4(Colors::Green)),
		// VColorData(XMFLOAT4(Colors::Black)),
		// VColorData(XMFLOAT4(Colors::Green)),
		// VColorData(XMFLOAT4(Colors::Green)),
		// VColorData(XMFLOAT4(Colors::Green)),
		// VColorData(XMFLOAT4(Colors::Green)),
		// VColorData(XMFLOAT4(Colors::Red)),

		// VColorData(XMFLOAT4(Colors::Black)),
		// VColorData(XMFLOAT4(Colors::Black)),
		// VColorData(XMFLOAT4(Colors::Black)),
		// VColorData(XMFLOAT4(Colors::Black)),
    };

    const std::array<std::uint16_t, 61> indices =
    {
    	// front face
    	0, 1, 2,
    	0, 2, 3,

    	// back face
    	4, 6, 5,
    	4, 7, 6,

    	// left face
    	4, 5, 1,
    	4, 1, 0,

    	// right face
    	3, 2, 6,
    	3, 6, 7,

    	// top face
    	1, 5, 6,
    	1, 6, 2,

    	// bottom face for pyramid and box
    	4, 0, 3,
    	4, 3, 7,

        // top of pyramid
        13, 0, 4,
        13, 3, 0,
        13, 7, 3,
        13, 4, 7,


        // Point
        8, 9, 10, 11,
    };
#pragma endregion
    constexpr auto vPosSize = verticesPos.size() * sizeof(VPosData);
    constexpr auto vColorSize = verticesColor.size() * sizeof(VColorData);
    constexpr auto indicesSize = indices.size() * sizeof(int16_t);
    // auto vSize = vertices.size() * sizeof(Vertex);

    ThrowIfFailed(D3DCreateBlob(vPosSize, m_vertexPosBufferData.GetAddressOf()));
    memcpy(m_vertexPosBufferData->GetBufferPointer(), verticesPos.data(), vPosSize);
    ThrowIfFailed(D3DCreateBlob(vColorSize, m_vertexColorBufferData.GetAddressOf()));
    memcpy(m_vertexColorBufferData->GetBufferPointer(), verticesColor.data(), vColorSize);
    ThrowIfFailed(D3DCreateBlob(indicesSize, m_indexBufferData.GetAddressOf()));
    memcpy(m_indexBufferData->GetBufferPointer(), indices.data(), indicesSize);
    // ThrowIfFailed(D3DCreateBlob(vSize, m_vertexBufferData.GetAddressOf()));
    // memcpy(m_vertexBufferData->GetBufferPointer(), vertices.data(), vSize);


    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));
    // Create default buffer
    m_vertexPosBufferGPU = createDefaultBuffer(m_device.Get(), m_commandList.Get(), verticesPos.data(), vPosSize, m_vertexPosBufferCPU);
    m_vertexColorBufferGPU = createDefaultBuffer(m_device.Get(), m_commandList.Get(), verticesColor.data(), vColorSize, m_vertexColorBufferCPU);
    m_indexBufferGPU  = createDefaultBuffer(m_device.Get(), m_commandList.Get(), indices.data(), indicesSize, m_indexBufferCPU);
    // m_vertexBufferGPU = createDefaultBuffer(m_device.Get(), m_commandList.Get(), vertices.data(), vSize, m_vertexBufferCPU);

    m_vertexPosStride     = sizeof(VPosData);
    m_vertexPosBufferSize = vPosSize;
    m_vertexColorStride     = sizeof(VColorData);
    m_vertexColorBufferSize = vColorSize;
    m_indexFormat      = DXGI_FORMAT_R16_UINT;
    m_indexBufferSize  = indicesSize;
    // m_vertexStride = sizeof(Vertex);
    // m_vertexBufferSize = vSize;

    m_indexCount = static_cast<int>(indices.size());
    m_startIndexLocation = 0;
    m_baseVertexLocation = 0;

    // Submit
    ThrowIfFailed(m_commandList->Close());
    m_commandQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList**>(m_commandList.GetAddressOf()));
    flushCommandQueue();

    m_vertexPosBufferCPU.Reset();
    m_vertexColorBufferCPU.Reset();
    m_indexBufferCPU.Reset();
    m_vertexColorBufferData.Reset();
    m_vertexPosBufferData.Reset();
    m_indexBufferData.Reset();
}

Box::~Box()
{
    m_constBuffer->Unmap(0, nullptr);
}

void Box::translation(float xx, float yy, float zz)
{
    float x = m_radius * sinf(m_phi) * cosf(m_theta);
    float z = m_radius * sinf(m_phi) * sinf(m_theta);
    float y = m_radius * cosf(m_phi);
    
    auto pos = XMVectorSet(x, y, z, 1.f);
    auto target = XMVectorZero();
    auto up = XMVectorSet(0.f, 1.f, 0.f, 0.f);
    
    auto view = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&m_view, view);

    auto world = XMLoadFloat4x4(&m_world);

    world *= XMMatrixTranslation(xx, yy, zz);

    auto proj = XMLoadFloat4x4(&m_proj);
    auto worldViewProj = world * view * proj;

    ObjectConstant objectConstant;
    XMStoreFloat4x4(&objectConstant.worldViewProj, XMMatrixTranspose(worldViewProj));
    objectConstant.time = m_timer.getTime();
    objectConstant.color = XMFLOAT4(Colors::Orange); 
    memcpy(m_mappedData, &objectConstant, sizeof(ObjectConstant));
}

void Box::update()
{
    float x = m_radius * sinf(m_phi) * cosf(m_theta);
    float z = m_radius * sinf(m_phi) * sinf(m_theta);
    float y = m_radius * cosf(m_phi);

    auto pos = XMVectorSet(x, y, z, 1.f);
    auto target = XMVectorZero();
    auto up = XMVectorSet(0.f, 1.f, 0.f, 0.f);
    
    auto view = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&m_view, view);

    auto world = XMLoadFloat4x4(&m_world);
    auto proj = XMLoadFloat4x4(&m_proj);
    auto worldViewProj = world * view * proj;

    ObjectConstant objectConstant;
    XMStoreFloat4x4(&objectConstant.worldViewProj, XMMatrixTranspose(worldViewProj));
    objectConstant.time = m_timer.getTime();
    memcpy(m_mappedData, &objectConstant, sizeof(ObjectConstant));
}

void Box::draw()
{
    ID3D12DescriptorHeap* descriptorHeaps[] = { m_cbvHeap.Get() };
    m_commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    D3D12_VERTEX_BUFFER_VIEW vertexView = {};
    vertexView.BufferLocation = m_vertexPosBufferGPU->GetGPUVirtualAddress();
    vertexView.StrideInBytes  = m_vertexPosStride;
    vertexView.SizeInBytes    = m_vertexPosBufferSize;
    // vertexView.BufferLocation = m_vertexBufferGPU->GetGPUVirtualAddress();
    // vertexView.StrideInBytes  = m_vertexStride;
    // vertexView.SizeInBytes    = m_vertexBufferSize;
    m_commandList->IASetVertexBuffers(0, 1, &vertexView);
    vertexView.BufferLocation = m_vertexColorBufferGPU->GetGPUVirtualAddress();
    vertexView.StrideInBytes  = m_vertexColorStride;
    vertexView.SizeInBytes    = m_vertexColorBufferSize;
    m_commandList->IASetVertexBuffers(1, 1, &vertexView);

    D3D12_INDEX_BUFFER_VIEW indexView = {};
    indexView.BufferLocation = m_indexBufferGPU->GetGPUVirtualAddress();
    indexView.Format         = m_indexFormat;
    indexView.SizeInBytes    = m_indexBufferSize;
    m_commandList->IASetIndexBuffer(&indexView);

    m_commandList->SetGraphicsRootDescriptorTable(0, m_cbvHeap->GetGPUDescriptorHandleForHeapStart());

    // m_commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    // m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    // m_commandList->DrawIndexedInstanced(3, 1, 0, 0, 0);
    // m_commandList->DrawInstanced(4, 1, 0, 0);

    // m_commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
    // m_commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    // m_commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
    // m_commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // translation(-1.f, 0.f, 0.f);
    m_commandList->DrawIndexedInstanced(36, 1, 0, 0, 0);
    // translation(+0.f, 0.f, 0.f);
    m_commandList->DrawIndexedInstanced(18, 1, 30, 0, 0);
}

void Box::onResize()
{
    DirectX12::onResize();

    auto p = XMMatrixPerspectiveFovLH(
        static_cast<float>(0.25f * std::numbers::pi), 
        static_cast<float>(m_width) / m_height,
        1.f,
        1000.f);
    XMStoreFloat4x4(&m_proj, p);
}