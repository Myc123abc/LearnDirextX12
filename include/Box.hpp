#pragma once

#include "DirectX12.hpp"

#include <vector>

struct Vertex
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT4 color;
};

struct ObjectConstant
{
    DirectX::XMFLOAT4X4 worldViewProj = createIdentity4x4();
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
    Microsoft::WRL::ComPtr<ID3D12Resource>       m_constBuffer; // Constant buffer use on CPU memory, which often changed in per frame
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_cbvHeap;     
    Microsoft::WRL::ComPtr<ID3D12RootSignature>  m_rootSignature;

    void* m_mappedData;

    Microsoft::WRL::ComPtr<ID3DBlob> m_vs;
    Microsoft::WRL::ComPtr<ID3DBlob> m_ps;
    std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;

    Microsoft::WRL::ComPtr<ID3DBlob> m_vertexBufferData;
    Microsoft::WRL::ComPtr<ID3DBlob> m_indexBufferData;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBufferGPU;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBufferGPU;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBufferCPU;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBufferCPU;

    int         m_vertexStride     = 0;
    int         m_vertexBufferSize = 0;
    DXGI_FORMAT m_indexFormat      = DXGI_FORMAT_R16_UINT;
    int         m_indexBufferSize  = 0;

    int m_indexCount         = 0;
    int m_startIndexLocation = 0;
    int m_baseVertexLocation = 0;
};