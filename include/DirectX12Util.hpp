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

    inline auto createIdentity4x4()
    {
        return DirectX::XMFLOAT4X4(
            1.f, 0.f, 0.f, 0.f,
            0.f, 1.f, 0.f, 0.f,
            0.f, 0.f, 1.f, 0.f,
            0.f, 0.f, 0.f, 1.f
        );
    }

    template<size_t Num>
    constexpr auto getMultiplesOf256()
    {
        return (Num + 255) & ~255;
    }

    Microsoft::WRL::ComPtr<ID3DBlob> loadBinaryFile(std::wstring_view filename);

    Microsoft::WRL::ComPtr<ID3D12Resource> createDefaultBuffer(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        const void* data,
        UINT64 size,
        Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);

    /*
    * Upload Buffer
    * Use for transform data from CPU to GPU (Upload Buffer to Constant Buffer)
    */
    template<typename T>
    class UploadBuffer
    {
    public:
        // So far, other situation not using for constant buffer is unknow.
        UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer = true)
        {
            m_elementSize = sizeof(T);

            if (isConstantBuffer)
                m_elementSize = getMultiplesOf256<sizeof(T)>();

            auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m_elementSize * elementCount);
            ThrowIfFailed(device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE, 
                &resourceDesc, 
                D3D12_RESOURCE_STATE_GENERIC_READ, 
                nullptr, 
                IID_PPV_ARGS(m_uploadBuffer.GetAddressOf())));

            ThrowIfFailed(m_uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedData)));
        }

        ~UploadBuffer()
        {
            if (m_uploadBuffer != nullptr)
                m_uploadBuffer->Unmap(0, nullptr);
        }

        UploadBuffer(const UploadBuffer&)            = delete;
        UploadBuffer(UploadBuffer&&)                 = delete;
        UploadBuffer& operator=(const UploadBuffer&) = delete;
        UploadBuffer& operator=(UploadBuffer&&)      = delete;

        ID3D12Resource* get() const { return m_uploadBuffer.Get(); }

        void copy(int elementIndex, const T& data)
        {
            memcpy(&m_mappedData[elementIndex * m_elementSize], &data, sizeof(T));
        }

    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> m_uploadBuffer;
        UINT  m_elementSize = 0;
        BYTE* m_mappedData  = nullptr;
    };
}