#include "HookManager.h"
#include "PresentHook.h"

namespace Hook
{
    bool InitializeAllHooks()
    {
        printf("[Hook] 正在初始化所有 Hook...\n");

        if (!PresentHook::Initialize())
        {
            printf("[-] PresentHook 初始化失败！\n");
            return false;
        }

        printf("[+] 所有 Hook 初始化成功！\n");
        return true;
    }

    void ShutdownAllHooks()
    {
        PresentHook::Shutdown();
        printf("[Hook] 所有 Hook 已卸载。\n");
    }
}