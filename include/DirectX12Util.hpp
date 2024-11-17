#pragma once

#include <DirectXMath.h>
#include <wrl.h>
#include <d3d12.h>
#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <d3dx12.h>
#include <windowsx.h>

#include <string>

#define ThrowIfFailed(x) if (FAILED(x)) throw std::exception();
#define ThrowIfFalse(x)  if (!x)        throw std::exception();

inline DirectX::XMFLOAT4X4 createIdentity4x4()
{
    return DirectX::XMFLOAT4X4(
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f
    );
}

inline int getMultiplesOf256(int num)
{
    return (num + 255) & ~255;
}

Microsoft::WRL::ComPtr<ID3DBlob> loadBinaryFile(const std::wstring& filename);

Microsoft::WRL::ComPtr<ID3D12Resource> createDefaultBuffer(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    const void* data,
    UINT64 size,
    Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);