//
// Created by Administrator on 2024/9/22.
//
#include "include/Base.h"
#include "include/DX11.h"
#include "Engine/Engine.h"
#include "Hook/HookManager.h"

static DWORD WINAPI InitializeThread(LPVOID)
{
    try
    {
        AllocConsole();
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
        freopen("CONOUT$", "w+", stdout);

        // 等待 Unity 运行环境稳定后再初始化，避免过早挂钩
        printf("[DLL] 等待游戏初始化...\n");
        for (int i = 0; i < 100; ++i)
        {
            if (FindWindowA("UnityWndClass", NULL) != NULL && GetModuleHandleA("GameAssembly.dll") != NULL)
            {
                break;
            }
            Sleep(100);
        }

        printf("[DLL] 开始初始化引擎...\n");
        Engine::Initialize();

        printf("[DLL] 开始初始化Hook...\n");
        if (!Hook::InitializeAllHooks())
        {
            printf("[-] Hook初始化失败！\n");
        }

        printf("[+] DLL初始化完成\n");
    }
    catch (const std::exception& ex)
    {
        printf("[-] 初始化线程C++异常: %s\n", ex.what());
    }
    catch (...)
    {
        printf("[-] 初始化线程未知异常\n");
    }
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, InitializeThread, nullptr, 0, nullptr);
    }
    return TRUE;
}
