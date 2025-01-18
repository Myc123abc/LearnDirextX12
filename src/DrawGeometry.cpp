#include "DrawGeometry.hpp"
#include "GeometryGenerator.h"
#include <random>

using namespace Microsoft::WRL;
using namespace DX;
using namespace DirectX;

namespace 
{

float getTerrainHeight(float x, float z)
{
    return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
}

}

DrawGeometry::DrawGeometry() {
  _wave = std::make_unique<Waves>(128, 128, 1.f, .03f, 4.f, .2f);

  // Root Signature
  // Defines what type of resources are bound to the graphics pipeline.
  buildRootSignature();

  // Shaders and layout
  // layout => vertex input structure
  buildShadersAndInputLayout();

  buildPipeline();

  ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));

  // Build vertices and indices.
  buildShapeGeometry();

  ThrowIfFailed(m_commandList->Close());
  m_commandQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList**>(m_commandList.GetAddressOf()));
  flushCommandQueue();

  buildConstantBufferResource();

  // Vertices only have grid which unchange, can reset.
  m_verticesUploadBuffer.Reset();
  // Indicies not change, can reset.
  m_indicesUploadBuffer.Reset();
}

void DrawGeometry::buildRootSignature() {
  // CD3DX12_DESCRIPTOR_RANGE descRange[2] = {};
  // descRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
  // descRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);
  
  CD3DX12_ROOT_PARAMETER rootPara[2] = {};
  // rootPara[0].InitAsDescriptorTable(1, &descRange[0]);
  // rootPara[1].InitAsDescriptorTable(1, &descRange[1]);
  rootPara[0].InitAsConstantBufferView(0);
  rootPara[1].InitAsConstantBufferView(1);

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
  int objectNum = m_renderItems.size();
  // m_frameHeapOffset = objectNum * FrameResourceNum;
  // int descriptorNum = m_frameHeapOffset + FrameResourceNum;

  // frame resource
  for (auto& frameResource : m_frameResources) {
    frameResource.objectUploadBuffer = std::make_unique<UploadBuffer<Object>>(m_device.Get(), objectNum);
    frameResource.frameUploadBuffer = std::make_unique<UploadBuffer<Frame>>(m_device.Get(), 1);

    frameResource.waveUploadBuffer = std::make_unique<UploadBuffer<Vertex>>(m_device.Get(), _wave->VertexCount(), false);

    ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(frameResource.cmdAlloc.GetAddressOf())));
  }

  // // descriptor heap
  // D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
  // heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  // heapDesc.NumDescriptors = descriptorNum;
  // heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  // heapDesc.NodeMask = 0;
  // ThrowIfFailed(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(m_cbvHeap.GetAddressOf())));

  // // descriptors
  // D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
  // desc.SizeInBytes = m_frameResources[0].objectUploadBuffer->getElementSize();
  // auto descriptor = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
  // for (const auto& frameResource : m_frameResources) {
  //   for (int i = 0; i < objectNum; ++i) {
  //     desc.BufferLocation = frameResource.objectUploadBuffer->getGPUAdd(i);
  //     m_device->CreateConstantBufferView(&desc, descriptor);
  //     descriptor.Offset(1, m_cbvSrvUavDescriptorSize);
  //   }
  // }

  // descriptor = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
  // descriptor.Offset(m_frameHeapOffset, m_cbvSrvUavDescriptorSize);
  // desc.SizeInBytes = m_frameResources[0].frameUploadBuffer->getElementSize();
  // for (const auto& frameResource : m_frameResources) {
  //   desc.BufferLocation = frameResource.frameUploadBuffer->getGPUAdd();
  //   m_device->CreateConstantBufferView(&desc, descriptor);
  // }
}

void DrawGeometry::buildShapeGeometry() {
  // Generate data
  GeometryGenerator geoGen;
  auto grid = geoGen.CreateGrid(160.f, 160.f, 50, 50);

  std::vector<Vertex> vertices;
  std::vector<uint16_t> indices;

  auto verticesSize = grid.Vertices.size();
  // the vertex data of wave is in frameResources
  assert(_wave->VertexCount() < 0x0000ffff);
  auto indicesSize = grid.Indices32.size() + _wave->TriangleCount() * 3;

  //
  // Vertices of Grid
  //
  vertices.reserve(verticesSize);
  for (const auto& gridVertex : grid.Vertices)
  {
      // vertices.emplace_back(Vertex{ vertex.Position, XMFLOAT4(Colors::Green) });
      Vertex vertex = { gridVertex.Position };
      vertex.pos.y = getTerrainHeight(vertex.pos.x, vertex.pos.z);

      if (vertex.pos.y < -10.f)
      {
          vertex.color = XMFLOAT4(1.f, .96f, .62f, 1.f);
      }
      else if (vertex.pos.y < 5.f)
      {
          vertex.color = XMFLOAT4(.48f, .77f, .46f, 1.f);
      }
      else if (vertex.pos.y < 12.f)
      {
          vertex.color = XMFLOAT4(.1f, .48f, .19f, 1.f);
      }
      else if (vertex.pos.y < 20.f)
      {
          vertex.color = XMFLOAT4(.45f, .39f, .34f, 1.f);
      }
      else 
      {
          vertex.color = XMFLOAT4(1.f, 1.f, 1.f, 1.f);
      }

      vertices.push_back(vertex);
  }

  indices.reserve(indicesSize);

  //
  // Indices of Grid
  //
  indices.append_range(grid.GetIndices16());

  //
  // Indices of Wave
  //
  indices.insert(indices.end(), indices.capacity() - indices.size(), 0);

	// Iterate over each quad.
	int m = _wave->RowCount();
	int n = _wave->ColumnCount();
	int k = grid.Indices32.size();
	for(int i = 0; i < m - 1; ++i)
	{
		for(int j = 0; j < n - 1; ++j)
		{
			indices[k] = i*n + j;
			indices[k + 1] = i*n + j + 1;
			indices[k + 2] = (i + 1)*n + j;

			indices[k + 3] = (i + 1)*n + j;
			indices[k + 4] = i*n + j + 1;
			indices[k + 5] = (i + 1)*n + j + 1;

			k += 6; // next quad
		}
	}

  //
  // Create GPU resources and save information of vertices and indicies 
  //
  m_verticesByteSize = vertices.size() * sizeof(Vertex);
  m_indicesByteSize = indices.size() * sizeof(uint16_t);

  m_verticesDefaultBuffer = createDefaultBuffer(m_device.Get(), m_commandList.Get(), vertices.data(), m_verticesByteSize, m_verticesUploadBuffer);
  m_indicesDefaultBuffer = createDefaultBuffer(m_device.Get(), m_commandList.Get(), indices.data(), m_indicesByteSize, m_indicesUploadBuffer);

  // Render Item
  // Describe data position
  RenderItem item;
  item.numFramesDirty = FrameResourceNum;

  // XMStoreFloat4x4(&item.objData.world, XMMatrixTranslation(0.f, -50.f, 0.f));
  item.objData.world = createIdentity4x4();
  item.index = 0;
  item.indicesSize = grid.Indices32.size();
  item.verticesOffset = 0;
  item.indicesOffset = 0;
  m_renderItems.push_back(item);

  item.index = 1;
  item.objData.world = createIdentity4x4();
  item.indicesSize = indicesSize - item.indicesSize;
  // Vertices of Wave is dynamic, so set -1 to represent it.
  item.verticesOffset = -1;
  item.indicesOffset = grid.Indices32.size();
  m_renderItems.push_back(item);
  
  // XMStoreFloat4x4(&item.objData.world, XMMatrixTranslation(0.f, 0.f, 0.f));
  // item.index = 1;
  // item.indicesSize = grid.Indices32.size();
  // item.verticesOffset = box.Vertices.size();
  // item.indicesOffset = box.Indices32.size();
  // m_renderItems.push_back(item);
}

void DrawGeometry::update() {
  DirectX12::update();

  Object object;
  XMMATRIX world;
  for (auto& item : m_renderItems) {
    if (item.numFramesDirty == 0) continue;
    --item.numFramesDirty;
    world = XMLoadFloat4x4(&item.objData.world);
    XMStoreFloat4x4(&object.world, XMMatrixTranspose(world));
    m_currentFrameResource->objectUploadBuffer->copy(item.index, object);
  }

  //
  // Update wave
  //
  // Every quarter second, generate a random wave.
	static float t_base = 0.0f;
	if((m_timer.getTime() - t_base) >= 0.25f)
	{
		t_base += 0.25f;

    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dist;
    static std::uniform_real_distribution<> fdist(.2f, .5f);
    
    dist.param(std::uniform_int_distribution<>::param_type(4, _wave->RowCount() - 5));
    int i = dist(gen);
    dist.param(std::uniform_int_distribution<>::param_type(4, _wave->ColumnCount() - 5));
    int j = dist(gen);

    float r = fdist(gen);

		_wave->Disturb(i, j, r);
	}

	// Update the wave simulation.
	_wave->Update(m_timer.getDelta());

	// Update the wave vertex buffer with the new solution.
	auto currWavesVB = m_currentFrameResource->waveUploadBuffer.get();
	for(int i = 0; i < _wave->VertexCount(); ++i)
	{
		Vertex v;

		v.pos = _wave->Position(i);
        v.color = XMFLOAT4(DirectX::Colors::Blue);

		currWavesVB->copy(i, v);
	}

	// Set the dynamic VB of the wave renderitem to the current frame VB.
	// mWavesRitem->Geo->VertexBufferGPU = currWavesVB->Resource();
}

void DrawGeometry::draw() {
  // ID3D12DescriptorHeap* descHeaps[] = { m_cbvHeap.Get() };
  // m_commandList->SetDescriptorHeaps(_countof(descHeaps), descHeaps);
  m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

  // Pass frame data
  // auto addr = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetGPUDescriptorHandleForHeapStart());
  // addr.Offset(m_frameHeapOffset, m_cbvSrvUavDescriptorSize);
  // m_commandList->SetGraphicsRootDescriptorTable(1, addr);
  m_commandList->SetGraphicsRootConstantBufferView(1, m_currentFrameResource->frameUploadBuffer->get()->GetGPUVirtualAddress());

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

  // addr = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetGPUDescriptorHandleForHeapStart());

  auto objByteSize = getMultiplesOf256<sizeof(Object)>();
  auto objAddr = m_currentFrameResource->objectUploadBuffer->get()->GetGPUVirtualAddress();
  for (const auto& item : m_renderItems) {
    // m_commandList->SetGraphicsRootDescriptorTable(0, addr);
    objAddr += item.index * objByteSize; 
    m_commandList->SetGraphicsRootConstantBufferView(0, objAddr);

    if (item.verticesOffset == -1)
    {
      auto tmp = vertexBufferView;
      vertexBufferView.BufferLocation = m_currentFrameResource->waveUploadBuffer->get()->GetGPUVirtualAddress();
      vertexBufferView.SizeInBytes = _wave->VertexCount() * sizeof(Vertex);
      m_commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
      m_commandList->DrawIndexedInstanced(item.indicesSize, 1, item.indicesOffset, 0, 0);
      continue;      
    }

    m_commandList->DrawIndexedInstanced(item.indicesSize, 1, item.indicesOffset, item.verticesOffset, 0);
    // addr.Offset(1, m_cbvSrvUavDescriptorSize);
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