#pragma once

#include "DirectX12.hpp"

struct Object {
  DirectX::XMFLOAT4X4 world;
};

struct Frame {
  DirectX::XMFLOAT4X4 viewProj;
};

struct Vertex {
  DirectX::XMFLOAT3 pos;
  DirectX::XMFLOAT4 color;
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
//   int m_vertexStride = 0;
  int m_vertexBufferSize = 0;
  Microsoft::WRL::ComPtr<ID3D12Resource> m_indicesDefaultBuffer;
  Microsoft::WRL::ComPtr<ID3D12Resource> m_indicesUploadBuffer;
  DXGI_FORMAT m_indexFormat = DXGI_FORMAT_R16_UINT;
  int m_indexBufferSize = 0;
  int m_indexCount = 0;
//   int m_startIndexLocation = 0;
};