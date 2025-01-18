#pragma once

#include "DirectX12.hpp"
#include "Wave.hpp"

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
  Microsoft::WRL::ComPtr<ID3D12Resource> m_verticesDefaultBuffer;
  Microsoft::WRL::ComPtr<ID3D12Resource> m_verticesUploadBuffer;
  Microsoft::WRL::ComPtr<ID3D12Resource> m_indicesDefaultBuffer;
  Microsoft::WRL::ComPtr<ID3D12Resource> m_indicesUploadBuffer;
  DXGI_FORMAT m_indexFormat = DXGI_FORMAT_R16_UINT;
  uint32_t m_verticesByteSize;
  uint32_t m_indicesByteSize;

  std::unique_ptr<Waves> _wave;
};