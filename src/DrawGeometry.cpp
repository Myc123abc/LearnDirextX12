#include "DrawGeometry.hpp"
#include "GeometryGenerator.h"

using namespace Microsoft::WRL;
using namespace DX;
using namespace DirectX;

DrawGeometry::DrawGeometry() {
  // Root Signature
  // Defines what type of resources are bound to the graphics pipeline.
  buildRootSignature();

  // Shaders and layout
  // layout => vertex input structure
  buildShadersAndInputLayout();

  buildConstantBufferResource();

  // TODO:: change to buildPipeline
  createPipeline();

  ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));

  // Build vertices and indices.
  buildShapeGeometry();

  ThrowIfFailed(m_commandList->Close());
  m_commandQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList**>(m_commandList.GetAddressOf()));
  flushCommandQueue();

  m_verticesUploadBuffer.Reset();
  m_indicesUploadBuffer.Reset();
}

void DrawGeometry::buildRootSignature() {
  // Object
  D3D12_DESCRIPTOR_RANGE descRange[2] = {};
  descRange[0].NumDescriptors = 1;
  descRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
  descRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
  descRange[0].BaseShaderRegister = 0;
  descRange[0].RegisterSpace = 0;

  // Frame
  descRange[1].NumDescriptors = 1;
  descRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
  descRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
  descRange[1].BaseShaderRegister = 1;
  descRange[1].RegisterSpace = 0;
  
  D3D12_ROOT_PARAMETER rootPara[2] = {};
  rootPara[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  rootPara[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
  rootPara[0].DescriptorTable.NumDescriptorRanges = 1;
  rootPara[0].DescriptorTable.pDescriptorRanges = &descRange[0];

  rootPara[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  rootPara[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
  rootPara[1].DescriptorTable.NumDescriptorRanges = 1;
  rootPara[1].DescriptorTable.pDescriptorRanges = &descRange[1];

  D3D12_ROOT_SIGNATURE_DESC sigDesc = {};
  sigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
  sigDesc.NumParameters = 2;
  sigDesc.pParameters = &rootPara[0];
  sigDesc.NumStaticSamplers = 0;
  sigDesc.pStaticSamplers = nullptr;

  ComPtr<ID3DBlob> serializedRootSig;
  ComPtr<ID3DBlob> errorBlob;
  HRESULT hr = D3D12SerializeRootSignature(&sigDesc, D3D_ROOT_SIGNATURE_VERSION_1, 
  serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());
  if (errorBlob != nullptr) {
    OutputDebugString((char*)(errorBlob->GetBufferPointer()));
    ThrowIfFalse(false);
  }
  ThrowIfFailed(hr);

  ThrowIfFailed(m_device->CreateRootSignature(
    0, 
    serializedRootSig->GetBufferPointer(), 
    serializedRootSig->GetBufferSize(), 
    IID_PPV_ARGS(m_rootSignature.GetAddressOf())));
}

void DrawGeometry::buildShadersAndInputLayout() {
  setShader("vs", L"../shader/drawGeo_vs.cso");
  setShader("ps", L"../shader/drawGeo_ps.cso");

  D3D12_INPUT_ELEMENT_DESC desc = {};
  desc.SemanticName = "POSITION";
  desc.SemanticIndex = 0;
  desc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
  desc.InputSlot = 0;
  // TODO: Test whether D3D12_APPEND_ALIGNED_ELEMENT can use different slots.
  desc.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
  desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
  desc.InstanceDataStepRate = 0;
  m_inputLayout.push_back(desc);

  desc.SemanticName = "COLOR";
  desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
  m_inputLayout.push_back(desc);
}

void DrawGeometry::buildShapeGeometry() {
  GeometryGenerator geoGen;
  auto box = geoGen.CreateBox(1.f, 1.f, 1.f, 0);

  std::vector<Vertex> vertices;
  std::vector<uint16_t> indices;

  vertices.reserve(box.Vertices.size());
  for (const auto& vertex : box.Vertices)
    vertices.emplace_back(Vertex{ vertex.Position, XMFLOAT4(Colors::Red) });
  indices.assign_range(box.GetIndices16());

  ComPtr<ID3DBlob> verticesData;
  ComPtr<ID3DBlob> indicesData;
  auto verticesSize = vertices.size() * sizeof(Vertex);
  auto indicesSize = indices.size() * sizeof(uint16_t);
  ThrowIfFailed(D3DCreateBlob(verticesSize, verticesData.GetAddressOf()));
  memcpy(verticesData->GetBufferPointer(), vertices.data(), verticesSize);
  ThrowIfFailed(D3DCreateBlob(indicesSize, indicesData.GetAddressOf()));
  memcpy(indicesData->GetBufferPointer(), indices.data(), indicesSize);

  m_verticesDefaultBuffer = createDefaultBuffer(m_device.Get(), m_commandList.Get(), verticesData.Get(), verticesSize, m_verticesUploadBuffer);
  m_indicesDefaultBuffer = createDefaultBuffer(m_device.Get(), m_commandList.Get(), indicesData.Get(), indicesSize, m_indicesUploadBuffer);

  m_vertexBufferSize = verticesSize;
  m_indexBufferSize = indicesSize;
  m_indexCount = static_cast<int>(indices.size());
}

void DrawGeometry::buildConstantBufferResource() {
  // Descriptor:
  //   Object for box resource
  //   Frame for global resource
  int descriptorNum = 2;

  m_objectUploadBuffer = std::make_unique<UploadBuffer<Object>>(m_device.Get(), 1);
  m_frameUploadBuffer = std::make_unique<UploadBuffer<Frame>>(m_device.Get(), 1);

  D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
  heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  heapDesc.NumDescriptors = descriptorNum;
  heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  heapDesc.NodeMask = 0;
  ThrowIfFailed(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(m_cbvHeap.GetAddressOf())));

  D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
  cbvDesc.BufferLocation = m_objectUploadBuffer->get()->GetGPUVirtualAddress();
  cbvDesc.SizeInBytes = getMultiplesOf256<sizeof(Object)>();
  auto cbvHeapAddr = m_cbvHeap->GetCPUDescriptorHandleForHeapStart();
  m_device->CreateConstantBufferView(&cbvDesc, cbvHeapAddr);
  cbvDesc.BufferLocation = m_frameUploadBuffer->get()->GetGPUVirtualAddress();
  cbvDesc.SizeInBytes = getMultiplesOf256<sizeof(Frame)>();
  cbvHeapAddr.ptr += m_cbvSrvUavDescriptorSize;
  m_device->CreateConstantBufferView(&cbvDesc, cbvHeapAddr);
}

void DrawGeometry::update() {
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
    auto viewProj = view * proj;

    Object object;
    XMStoreFloat4x4(&object.world, XMMatrixTranspose(world));
    m_objectUploadBuffer->copy(0, object);

    Frame frame;
    XMStoreFloat4x4(&frame.viewProj, XMMatrixTranspose(viewProj));
    m_frameUploadBuffer->copy(0, frame);
}

void DrawGeometry::draw() {
  ID3D12DescriptorHeap* descHeaps[] = { m_cbvHeap.Get() };
  m_commandList->SetDescriptorHeaps(_countof(descHeaps), descHeaps);
  m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

  // Pass frame data
  auto addr = m_cbvHeap->GetGPUDescriptorHandleForHeapStart();
  addr.ptr += m_cbvSrvUavDescriptorSize;
  m_commandList->SetGraphicsRootDescriptorTable(1, addr);

  // Pass object data
  D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
  vertexBufferView.BufferLocation = m_verticesDefaultBuffer->GetGPUVirtualAddress();
  vertexBufferView.StrideInBytes = sizeof(Vertex);
  vertexBufferView.SizeInBytes = m_vertexBufferSize;
  m_commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

  D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
  indexBufferView.BufferLocation = m_indicesDefaultBuffer->GetGPUVirtualAddress();
  indexBufferView.Format = m_indexFormat;
  indexBufferView.SizeInBytes = m_indexBufferSize;
  m_commandList->IASetIndexBuffer(&indexBufferView);

  m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  addr.ptr -= m_cbvSrvUavDescriptorSize;
  m_commandList->SetGraphicsRootDescriptorTable(0, addr);
  m_commandList->DrawIndexedInstanced(m_indexCount, 1, 0, 0, 0);
}

void DrawGeometry::onResize() {
    DirectX12::onResize();

    auto p = XMMatrixPerspectiveFovLH(
        static_cast<float>(0.25f * std::numbers::pi), 
        static_cast<float>(m_width) / m_height,
        1.f,
        1000.f);
    XMStoreFloat4x4(&m_proj, p);
}