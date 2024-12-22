#pragma once

#include "DirectX12.hpp"

struct Object {
  DirectX::XMFLOAT4X4 world = DX::createIdentity4x4();
};

struct Frame {
  DirectX::XMFLOAT4X4 viewProj;
};

struct Vertex {
  DirectX::XMFLOAT3 pos;
  DirectX::XMFLOAT4 color;
};

struct RenderItem {
  Object objData;
  uint32_t index;
  uint32_t verticesOffset;
  uint32_t indicesOffset;
  uint32_t indicesSize;
};

class DrawGeometry : public DirectX12 {
public:
  DrawGeometry();
  ~DrawGeometry() { if (m_device) flushCommandQueue(); }

private:
  void buildRootSignature();
  void buildShadersAndInputLayout();
  void buildShapeGeometry();
  void buildConstantBufferResource();

  void update() override;
  void draw() override;
  void onResize() override;

private:
  std::unique_ptr<DX::UploadBuffer<Object>> m_objectUploadBuffer;
  std::unique_ptr<DX::UploadBuffer<Frame>> m_frameUploadBuffer;
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_cbvHeap; 

  Microsoft::WRL::ComPtr<ID3D12Resource> m_verticesDefaultBuffer;
  Microsoft::WRL::ComPtr<ID3D12Resource> m_verticesUploadBuffer;
  Microsoft::WRL::ComPtr<ID3D12Resource> m_indicesDefaultBuffer;
  Microsoft::WRL::ComPtr<ID3D12Resource> m_indicesUploadBuffer;
  DXGI_FORMAT m_indexFormat = DXGI_FORMAT_R16_UINT;
  uint32_t m_verticesByteSize;
  uint32_t m_indicesByteSize;

  std::vector<RenderItem> m_renderItems;
  uint32_t m_frameHeapOffset = 0;
};