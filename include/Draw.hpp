#include "FrameResource.hpp"
#include "DirectX12.hpp"
#include "Geometry.hpp"
#include <array>

struct Vertex
{
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT4 col;
};

class Draw : public DirectX12 
{
public:
    Draw();

    void update()   override;
    void draw()     override;
    void onResize() override;

private:
    struct RenderItem
    {
        DirectX::XMFLOAT4X4 world = DX::createIdentity4x4(); 
        int numFramesDirty = NumberOfFrameResources;   
        int objCBIndex = -1;

        DX::MeshGeometry* geo = nullptr;
        D3D12_PRIMITIVE_TOPOLOGY primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

        int indexCount = 0;
        int startIndexLocation = 0;
        int baseVertexLocation = 0;
    };

private:
    void buildGeometry();
    void buildRenderItems();
    void buildDescriptorHeaps();
    void buildConstantBufferViews();

    void drawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& renderItems);

    void tmpFunc_drawEnd() override;

private:
    std::vector<std::unique_ptr<RenderItem>> m_renderItems;

    std::vector<RenderItem*> m_opaqueRenderItems;
    std::vector<RenderItem*> m_transparentRenderItems;

    static constexpr int NumberOfFrameResources = 3;
    FrameResource* m_currentFrameResource       = nullptr;
    int            m_currentFrameResourceIndex  = 0;
    std::array<std::unique_ptr<FrameResource>, NumberOfFrameResources> m_frameResources;

    FrameData m_frameData;
    DirectX::XMFLOAT3 m_cameraPos;

    std::unordered_map<std::string, std::unique_ptr<DX::MeshGeometry>> m_geometries;

    uint32_t m_frameDataOffset = 0;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_cbvHeap;
};