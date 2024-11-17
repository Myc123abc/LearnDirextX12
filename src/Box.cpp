#include "Box.hpp"

#include <DirectXColors.h>

#include <array>
#include <numbers>

using namespace Microsoft::WRL;
using namespace DirectX::Colors;
using namespace DirectX;

Box::Box()
{
    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));

    // ----------------------------------
    //  Create constant buffer resources
    // ----------------------------------

    // Hardware requires multiples of 256 for constant data
    int size = getMultiplesOf256(sizeof(ObjectConstant));
    
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


    // --------------------
    //  Set root signature
    // --------------------
    
    D3D12_DESCRIPTOR_RANGE cbvTable = {};
    cbvTable.RangeType      = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    cbvTable.NumDescriptors = 1;
    cbvTable.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER rootParameter[1] = {};
    rootParameter[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; 
    rootParameter[0].DescriptorTable.NumDescriptorRanges = cbvTable.NumDescriptors;
    rootParameter[0].DescriptorTable.pDescriptorRanges   = &cbvTable;
    
    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.NumParameters = 1;
    rootSignatureDesc.pParameters   = rootParameter;
    rootSignatureDesc.Flags         = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> serializedRootSignature;
    ComPtr<ID3DBlob> errorBlob;
    auto hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1,
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


    // --------
    //  Layout
    // --------

    m_vs = loadBinaryFile(L"../shader/box_vs.cso");
    m_ps = loadBinaryFile(L"../shader/box_ps.cso");

    m_inputLayout =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    // ----------------------
    //  Vertices and Indices
    // ----------------------

    const std::array<Vertex, 8> vertices =
    {
        Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
		Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) })
    };

    const std::array<std::uint16_t, 36> indices =
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

    	// bottom face
    	4, 0, 3,
    	4, 3, 7
    };

    constexpr auto verticesSize = vertices.size() * sizeof(Vertex);
    constexpr auto indicesSize = indices.size() * sizeof(int16_t);

    ThrowIfFailed(D3DCreateBlob(verticesSize, m_vertexBufferData.GetAddressOf()));
    memcpy(m_vertexBufferData->GetBufferPointer(), vertices.data(), verticesSize);
    ThrowIfFailed(D3DCreateBlob(indicesSize, m_indexBufferData.GetAddressOf()));
    memcpy(m_indexBufferData->GetBufferPointer(), indices.data(), indicesSize);


    // Create default buffer
    m_vertexBufferGPU = createDefaultBuffer(m_device.Get(), m_commandList.Get(), vertices.data(), verticesSize, m_vertexBufferCPU);
    m_indexBufferGPU  = createDefaultBuffer(m_device.Get(), m_commandList.Get(), indices.data(), indicesSize, m_indexBufferCPU);

    m_vertexStride     = sizeof(Vertex);
    m_vertexBufferSize = verticesSize;
    m_indexFormat      = DXGI_FORMAT_R16_UINT;
    m_indexBufferSize  = indicesSize;

    m_indexCount = static_cast<int>(indices.size());
    m_startIndexLocation = 0;
    m_baseVertexLocation = 0;


    // -----------------------
    //  Create Pipeline state
    // -----------------------

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout    = { m_inputLayout.data(), static_cast<UINT>(m_inputLayout.size()) };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = 
    {
        reinterpret_cast<BYTE*>(m_vs->GetBufferPointer()),
        m_vs->GetBufferSize()
    };
    psoDesc.PS =
    {
        reinterpret_cast<BYTE*>(m_ps->GetBufferPointer()),
        m_ps->GetBufferSize()
    };
    psoDesc.RasterizerState       = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState     = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask            = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets      = 1;
    psoDesc.RTVFormats[0]         = m_backBufferFormat;
    psoDesc.SampleDesc.Count      = 1;
    psoDesc.SampleDesc.Quality    = 0;
    psoDesc.DSVFormat             = m_depthBufferFormat;
    ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pso)));

    // Submit
    ThrowIfFailed(m_commandList->Close());
    m_commandQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList**>(m_commandList.GetAddressOf()));
    flushCommandQueue();
}

Box::~Box()
{
    m_constBuffer->Unmap(0, nullptr);
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
    memcpy(m_mappedData, &objectConstant, sizeof(ObjectConstant));
}

void Box::draw()
{
    ID3D12DescriptorHeap* descriptorHeaps[] = { m_cbvHeap.Get() };
    m_commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    D3D12_VERTEX_BUFFER_VIEW vertexView = {};
    vertexView.BufferLocation = m_vertexBufferGPU->GetGPUVirtualAddress();
    vertexView.StrideInBytes  = m_vertexStride;
    vertexView.SizeInBytes    = m_vertexBufferSize;
    m_commandList->IASetVertexBuffers(0, 1, &vertexView);

    D3D12_INDEX_BUFFER_VIEW indexView = {};
    indexView.BufferLocation = m_indexBufferGPU->GetGPUVirtualAddress();
    indexView.Format         = m_indexFormat;
    indexView.SizeInBytes    = m_indexBufferSize;
    m_commandList->IASetIndexBuffer(&indexView);

    m_commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    m_commandList->SetGraphicsRootDescriptorTable(0, m_cbvHeap->GetGPUDescriptorHandleForHeapStart());

    m_commandList->DrawIndexedInstanced(m_indexCount, 1, 0, 0, 0);
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