#include "DirectX12.hpp"
#include <memory>

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd)
{
    auto dx12 = std::make_unique<DirectX12>(800, 600);
    dx12->run();
}