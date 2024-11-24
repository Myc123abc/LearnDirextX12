#pragma once

#include "DirectX12.hpp"

#include <vector>
#include <DirectXPackedVector.h>

struct Vertex
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT4 color;
};
struct VPosData
{
    DirectX::XMFLOAT3 pos;
};
struct VColorData
{
    // DirectX::XMFLOAT4 color;
    DirectX::PackedVector::XMCOLOR color;
};

struct ObjectConstant
{
    // 0 ~ 63
    DirectX::XMFLOAT4X4 worldViewProj = createIdentity4x4();
    // 64 ~ 73
    uint32_t matIndex = 0;
    float time = 0.f;
    float padding1[2];
    // 74 ~ 90
    DirectX::XMFLOAT4 color = {};
};

class Box : public DirectX12
{
public:
    Box();
    ~Box();

protected:
    virtual void update()   override;
    virtual void draw()     override;
    virtual void onResize() override;

private:
    void translation(float x, float y, float z);

private:
    Microsoft::WRL::ComPtr<ID3D12Resource>       m_constBuffer; // Constant buffer use on CPU memory, which often changed in per frame
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_cbvHeap;     
    Microsoft::WRL::ComPtr<ID3D12RootSignature>  m_rootSignature;

    void* m_mappedData;

    Microsoft::WRL::ComPtr<ID3DBlob> m_vs;
    Microsoft::WRL::ComPtr<ID3DBlob> m_ps;
    std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;

    Microsoft::WRL::ComPtr<ID3DBlob> m_vertexBufferData;
    Microsoft::WRL::ComPtr<ID3DBlob> m_vertexPosBufferData;
    Microsoft::WRL::ComPtr<ID3DBlob> m_vertexColorBufferData;
    Microsoft::WRL::ComPtr<ID3DBlob> m_indexBufferData;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBufferGPU;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexPosBufferGPU;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexColorBufferGPU;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBufferGPU;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBufferCPU;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexPosBufferCPU;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexColorBufferCPU;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBufferCPU;

    int         m_vertexStride     = 0;
    int         m_vertexBufferSize = 0;
    int         m_vertexPosStride     = 0;
    int         m_vertexPosBufferSize = 0;
    int         m_vertexColorStride     = 0;
    int         m_vertexColorBufferSize = 0;
    DXGI_FORMAT m_indexFormat      = DXGI_FORMAT_R16_UINT;
    int         m_indexBufferSize  = 0;

    int m_indexCount         = 0;
    int m_startIndexLocation = 0;
    int m_baseVertexLocation = 0;
};