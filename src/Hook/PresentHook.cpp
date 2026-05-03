// ============================================================================
// 文件：src/Hook/PresentHook.cpp
// 模块：Hook 层 - DX11 Present Hook 包装器实现
// ============================================================================

#include "PresentHook.h"
#include "DX11Hook.h"
#include "Core/globals.h"
#include <cstdio>

namespace PresentHook
{
    bool Initialize()
    {
        if (!DX11HOOK())
        {
            printf("[-] PresentHook: DX11 Hook 安装失败\n");
            return false;
        }

        printf("[+] PresentHook: DX11 Hook 安装成功\n");
        return true;
    }

    void Shutdown()
    {
        // 虚表 Hook 通过进程退出自动清理。
        // 如果需要主动卸载（如热重载场景），在此处恢复原始虚表条目。
        printf("[PresentHook] Shutdown\n");
    }

    IDXGISwapChain* GetSwapChain()
    {
        return g_pSwapChain;
    }

} // namespace PresentHook
