#include "PresentHook.h"
#include "include/DX11.h"          // 调用你原来的 DX11HOOK()

extern IDXGISwapChain* g_pSwapChain;

namespace PresentHook
{
    bool Initialize()
    {
        if (!DX11HOOK())
        {
            printf("[-] DX11 Hook 初始化失败\n");
            return false;
        }

        printf("[PresentHook] DX11 Hook 初始化完成\n");
        return true;
    }

    void Shutdown() { }

    IDXGISwapChain* GetSwapChain()
    {
        return g_pSwapChain;
    }
}
