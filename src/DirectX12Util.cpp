#include <fstream>

using namespace Microsoft::WRL;
using namespace DX;

ComPtr<ID3DBlob> DX::loadBinaryFile(std::wstring_view filename)
{
    std::ifstream ifile(filename.data(), std::ios::binary);

    ifile.seekg(0, std::ios_base::end);
    auto size = static_cast<int>(ifile.tellg());
    ifile.seekg(0, std::ios_base::beg);

    ComPtr<ID3DBlob> blob;
    ThrowIfFailed(D3DCreateBlob(size, blob.GetAddressOf()));

    ifile.read(reinterpret_cast<char*>(blob->GetBufferPointer()), size); 
    ifile.close();

    return blob;
}

Microsoft::WRL::ComPtr<ID3D12Resource> DX::createDefaultBuffer(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    const void* data,
    UINT64 size,
    Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer)
{
    ComPtr<ID3D12Resource> defaultBuffer;

    D3D12_HEAP_PROPERTIES heapProp = {};
    heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
    auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(size);

    ThrowIfFailed(device->CreateCommittedResource(
        &heapProp, 
        D3D12_HEAP_FLAG_NONE, 
        &resourceDesc,
        D3D12_RESOURCE_STATE_COMMON, 
        nullptr, 
        IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

    heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

    ThrowIfFailed(device->CreateCommittedResource(
        &heapProp, 
        D3D12_HEAP_FLAG_NONE, 
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, 
        nullptr, 
        IID_PPV_ARGS(uploadBuffer.GetAddressOf())));
    
    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = data;
    subResourceData.RowPitch = size;
    subResourceData.SlicePitch = size;

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Transition.pResource   = defaultBuffer.Get();
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST; 
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmdList->ResourceBarrier(1, &barrier); 
    
    UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);

    barrier.Transition.StateBefore = barrier.Transition.StateAfter;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_GENERIC_READ;
    cmdList->ResourceBarrier(1, &barrier);

    return defaultBuffer;
}