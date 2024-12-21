#define D

#ifdef D
#include "DrawGeometry.hpp"
#else
#include "Box.hpp"
#endif

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nShowCmd);
#ifdef D
    DrawGeometry().run();
#else
    // DirectX12().run();
    Box().run();
#endif
}