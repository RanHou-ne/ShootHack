// ============================================================================
// 文件：src/Hook/HookManager.cpp
// 模块：Hook 层 - Hook 管理器实现
// ============================================================================

#include "HookManager.h"
#include "PresentHook.h"
#include <cstdio>

namespace Hook
{
    bool InitializeAllHooks()
    {
        printf("[Hook] 正在初始化所有 Hook...\n");

        // ---- Present Hook（DX11 虚表 Hook）----
        if (!PresentHook::Initialize())
        {
            printf("[-] PresentHook 初始化失败！\n");
            return false;
        }

        // ---- 在此处添加更多 Hook ----
        // 示例：
        // if (!DrawHook::Initialize())
        // {
        //     printf("[-] DrawHook 初始化失败！\n");
        //     return false;
        // }

        printf("[+] 所有 Hook 初始化成功！\n");
        return true;
    }

    void ShutdownAllHooks()
    {
        PresentHook::Shutdown();

        // ---- 在此处卸载更多 Hook ----
        // DrawHook::Shutdown();

        printf("[Hook] 所有 Hook 已卸载。\n");
    }

} // namespace Hook
