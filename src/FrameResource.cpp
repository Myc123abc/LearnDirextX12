#include "FrameResource.hpp"

using namespace DX;

FrameResource::FrameResource(ID3D12Device* device, int objectNum, int frameNum)
{
    ThrowIfFailed(device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(cmdAlloc.GetAddressOf())));

    objectInfo = std::make_unique<UploadBuffer<ObjectInfo>>(device, objectNum);
    frameData  = std::make_unique<UploadBuffer<FrameData>>(device, frameNum);
}