#pragma once

namespace DX
{
    class com_exception : public std::exception
    {
    public:
        com_exception(HRESULT hr) noexcept : result(hr) {}

        const char* what() const override
        {
            static char s_str[64] = {};
            sprintf_s(s_str, "Failure with HRSULT of %08X", static_cast<uint8_t>(result));
            return s_str;
        }

    private:
        HRESULT result;
    };

    inline void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            throw com_exception(hr);
        }
    }

    inline void ThrowIfFalse(bool x)  
    {
        if (!x) 
        {
            throw std::exception();
        }
    }

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

}