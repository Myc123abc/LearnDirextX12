#pragma once

struct ObjectInfo
{
    DirectX::XMFLOAT4X4 world = DX::createIdentity4x4();
};

struct FrameData
{
    DirectX::XMFLOAT4X4 view        = DX::createIdentity4x4();
    DirectX::XMFLOAT4X4 invView     = DX::createIdentity4x4();
    DirectX::XMFLOAT4X4 proj        = DX::createIdentity4x4();
    DirectX::XMFLOAT4X4 invProj     = DX::createIdentity4x4();
    DirectX::XMFLOAT4X4 viewProj    = DX::createIdentity4x4();
    DirectX::XMFLOAT4X4 invViewProj = DX::createIdentity4x4();
    DirectX::XMFLOAT3   cameraPos   = {};
    float               pad0        = 0.f; // Promise 16bytes alignment
    DirectX::XMFLOAT2   renderTargetSize    = {};
    DirectX::XMFLOAT2   invRenderTargetSize = {};
    float               nearZ = 0.f;
    float               farZ  = 0.f;
    float               totalTime = 0.f;
    float               deltaTime = 0.f;
};

class FrameResource
{
public:
    FrameResource(ID3D12Device* device, int objectNum, int frameNum);
    ~FrameResource() = default;

    FrameResource(const FrameResource&)            = delete;
    FrameResource(FrameResource&&)                 = delete;
    FrameResource& operator=(const FrameResource&) = delete;
    FrameResource& operator=(FrameResource&&)      = delete;

    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmdAlloc;

    UINT64 fence = 0;

    std::unique_ptr<DX::UploadBuffer<ObjectInfo>> objectInfo;
    std::unique_ptr<DX::UploadBuffer<FrameData>>  frameData;
};