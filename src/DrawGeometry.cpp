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

  buildPipeline();

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
  CD3DX12_DESCRIPTOR_RANGE descRange[2] = {};
  descRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
  descRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);
  
  CD3DX12_ROOT_PARAMETER rootPara[2] = {};
  rootPara[0].InitAsDescriptorTable(1, &descRange[0]);
  rootPara[1].InitAsDescriptorTable(1, &descRange[1]);

  CD3DX12_ROOT_SIGNATURE_DESC sigDesc = {};
  sigDesc.Init(2, rootPara, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

  DirectX12::buildRootSignature(sigDesc);
}

void DrawGeometry::buildShadersAndInputLayout() {
  setShader("vs", L"../shader/drawGeo_vs.cso");
  setShader("ps", L"../shader/drawGeo_ps.cso");

  m_inputLayout = {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
  };
}

void DrawGeometry::buildConstantBufferResource() {
  int objectNum = 2;
  int frameNum  = 1;
  int descriptorNum = objectNum + frameNum;
  m_frameHeapOffset = objectNum;

  // frame resource
  // TODO: change to 3 frames
  m_objectUploadBuffer = std::make_unique<UploadBuffer<Object>>(m_device.Get(), objectNum);
  m_frameUploadBuffer = std::make_unique<UploadBuffer<Frame>>(m_device.Get(), 1);

  // descriptor heap
  D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
  heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  heapDesc.NumDescriptors = descriptorNum;
  heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  heapDesc.NodeMask = 0;
  ThrowIfFailed(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(m_cbvHeap.GetAddressOf())));

  // descriptors
  D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
  desc.SizeInBytes = m_objectUploadBuffer->getElementSize();
  auto descriptor = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
  for (int i = 0; i < objectNum; ++i) {
    desc.BufferLocation = m_objectUploadBuffer->getGPUAdd(i);
    m_device->CreateConstantBufferView(&desc, descriptor);
    descriptor.Offset(1, m_cbvSrvUavDescriptorSize);
  }

  desc.BufferLocation = m_frameUploadBuffer->getGPUAdd();
  desc.SizeInBytes = m_frameUploadBuffer->getElementSize();
  m_device->CreateConstantBufferView(&desc, descriptor);
}

void DrawGeometry::buildShapeGeometry() {
  GeometryGenerator geoGen;
  auto box = geoGen.CreateBox(2.f, 1.f, 1.f, 0);
  auto box2 = geoGen.CreateBox(1.f, 2.f, 1.f, 0);
  // TODO: tow box share the vertices

  std::vector<Vertex> vertices;
  std::vector<uint16_t> indices;

  auto verticesSize = box.Vertices.size() + box2.Vertices.size();
  auto indicesSize = box.Indices32.size() + box2.Indices32.size();

  vertices.reserve(verticesSize);
  for (const auto& vertex : box.Vertices)
    vertices.emplace_back(Vertex{ vertex.Position, XMFLOAT4(Colors::Red) });
  for (const auto& vertex : box2.Vertices)
    vertices.emplace_back(Vertex{ vertex.Position, XMFLOAT4(Colors::Green) });

  indices.reserve(indicesSize);
  indices.append_range(box.GetIndices16());
  indices.append_range(box2.GetIndices16());

  m_verticesByteSize = vertices.size() * sizeof(Vertex);
  m_indicesByteSize = indices.size() * sizeof(uint16_t);

  m_verticesDefaultBuffer = createDefaultBuffer(m_device.Get(), m_commandList.Get(), vertices.data(), m_verticesByteSize, m_verticesUploadBuffer);
  m_indicesDefaultBuffer = createDefaultBuffer(m_device.Get(), m_commandList.Get(), indices.data(), m_indicesByteSize, m_indicesUploadBuffer);

  RenderItem item;

  XMStoreFloat4x4(&item.objData.world, XMMatrixTranslation(2.f, 0.f, 0.f));
  item.index = 0;
  item.indicesSize = box.Indices32.size();
  item.verticesOffset = 0;
  item.indicesOffset = 0;
  m_renderItems.push_back(item);
  
  XMStoreFloat4x4(&item.objData.world, XMMatrixTranslation(-2.f, 0.f, 0.f));
  item.index = 1;
  item.indicesSize = box2.Indices32.size();
  item.verticesOffset = box.Vertices.size();
  item.indicesOffset = box.Indices32.size();
  m_renderItems.push_back(item);
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

  auto proj = XMLoadFloat4x4(&m_proj);
  auto viewProj = view * proj;

  Object object;
  XMMATRIX world;
  for (const auto& item : m_renderItems) {
    world = XMLoadFloat4x4(&item.objData.world);
    XMStoreFloat4x4(&object.world, XMMatrixTranspose(world));
    m_objectUploadBuffer->copy(item.index, object);
  }

  Frame frame;
  XMStoreFloat4x4(&frame.viewProj, XMMatrixTranspose(viewProj));
  m_frameUploadBuffer->copy(0, frame);
}

void DrawGeometry::draw() {
  ID3D12DescriptorHeap* descHeaps[] = { m_cbvHeap.Get() };
  m_commandList->SetDescriptorHeaps(_countof(descHeaps), descHeaps);
  m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

  // Pass frame data
  auto addr = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetGPUDescriptorHandleForHeapStart());
  addr.Offset(m_frameHeapOffset, m_cbvSrvUavDescriptorSize);
  m_commandList->SetGraphicsRootDescriptorTable(1, addr);

  m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  // Pass object data
  D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
  vertexBufferView.BufferLocation = m_verticesDefaultBuffer->GetGPUVirtualAddress();
  vertexBufferView.StrideInBytes = sizeof(Vertex);
  vertexBufferView.SizeInBytes = m_verticesByteSize;
  m_commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

  D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
  indexBufferView.BufferLocation = m_indicesDefaultBuffer->GetGPUVirtualAddress();
  indexBufferView.Format = m_indexFormat;
  indexBufferView.SizeInBytes = m_indicesByteSize;
  m_commandList->IASetIndexBuffer(&indexBufferView);

  addr = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetGPUDescriptorHandleForHeapStart());

  for (const auto& item : m_renderItems) {
    m_commandList->SetGraphicsRootDescriptorTable(0, addr);
    m_commandList->DrawIndexedInstanced(item.indicesSize, 1, item.indicesOffset, item.verticesOffset, 0);
    addr.Offset(1, m_cbvSrvUavDescriptorSize);
  }
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