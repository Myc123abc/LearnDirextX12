#include "DirectX12.hpp"

#include <exception>

using namespace Microsoft::WRL;

#define ThrowIfFailed(x) if (FAILED(x)) throw std::exception();

DirectX12::DirectX12(HWND hWnd, int width, int height)
    : m_width(width), m_height(height)
{
#ifdef _DEBUG
    {
        // Enable D3D12 debug layer
        ComPtr<ID3D12Debug> debugController;
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf())));
        debugController->EnableDebugLayer();
    }

    // ---------------
    // Create factory
    // ---------------

    // Enable debug feature of factory
    ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(m_factory.GetAddressOf())));
#else
    ThrowIfFailed(CreateDXGIFactory(IID_PPV_ARGS(m_factory.GetAddressOf())));
#endif
}

void DirectX12::render()
{

}

