// ============================================================================
// 文件：src/dllmain.cpp
// 模块：入口层 - DLL 主入口
// ============================================================================
// 【用途】
//   DLL 注入后的入口点。在 DLL_PROCESS_ATTACH 时创建初始化线程，
//   等待 Unity 运行环境稳定后依次初始化引擎和 Hook 系统。
//
// 【初始化流程】
//   DllMain (DLL_PROCESS_ATTACH)
//     → CreateThread(InitializeThread)
//       → 等待 Unity 窗口和 GameAssembly.dll 就绪
//       → Engine::Initialize()    （绑定 IL2CPP API）
//       → Hook::InitializeAllHooks()  （安装 DX11 虚表 Hook）
//         → 首次 hkPresent() 调用时初始化 ImGui + 菜单
//
// 【扩展指南】
//   如果需要在初始化时执行额外操作（如加载外部配置、连接服务器等）：
//   1. 在 InitializeThread 中 Hook::InitializeAllHooks() 之后添加代码
//   2. 保持异常处理，避免未捕获异常导致游戏崩溃
// ============================================================================

#include "Core/Base.h"
#include "Engine/Engine.h"
#include "Hook/HookManager.h"

// ============================================================================
// InitializeThread —— 异步初始化线程
// ============================================================================
// 在独立线程中执行，避免阻塞 DllMain（DllMain 中不能做耗时操作）。

static DWORD WINAPI InitializeThread(LPVOID)
{
    try
    {
        // ---- 分配调试控制台 ----
        AllocConsole();
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
        freopen("CONOUT$", "w+", stdout);

        printf("============================================\n");
        printf("  ShootHack DLL 已注入\n");
        printf("============================================\n");

        // ---- 等待 Unity 运行环境稳定 ----
        // Unity 游戏启动时需要一段时间初始化窗口和加载 GameAssembly.dll。
        // 过早 Hook 会导致崩溃，因此轮询等待两个条件同时满足：
        //   1. UnityWndClass 窗口已创建
        //   2. GameAssembly.dll 已加载到进程
        printf("[DLL] 等待游戏初始化...\n");
        for (int i = 0; i < 200; ++i)
        {
            const bool windowReady   = (FindWindowA("UnityWndClass", nullptr) != nullptr);
            const bool assemblyReady = (GetModuleHandleA("GameAssembly.dll") != nullptr);

            if (windowReady && assemblyReady)
            {
                printf("[DLL] 游戏环境就绪（等待了 %d * 100ms）\n", i);
                break;
            }
            Sleep(100);
        }

        // ---- 初始化引擎层（绑定 IL2CPP API）----
        printf("[DLL] 初始化引擎层...\n");
        Engine::Initialize();

        // ---- GOM 遍历：输出所有游戏对象地址和名称 ----
        printf("[DLL] 等待场景加载后遍历 GOM...\n");
        Sleep(2000);  // 等待 2 秒让场景加载
        Engine::TraverseGOM(0x17C3F18, 0);  // 遍历所有对象

        // ---- 初始化 Hook 系统（安装 DX11 虚表 Hook）----
        printf("[DLL] 初始化 Hook 系统...\n");
        if (!Hook::InitializeAllHooks())
        {
            printf("[-] Hook 初始化失败！DLL 功能将不可用。\n");
            return 1;
        }

        printf("[+] DLL 初始化完成，按 INSERT 键打开/关闭菜单\n");
        printf("============================================\n");
    }
    catch (const std::exception& ex)
    {
        printf("[-] 初始化线程 C++ 异常: %s\n", ex.what());
    }
    catch (...)
    {
        printf("[-] 初始化线程未知异常\n");
    }

    return 0;
}

// ============================================================================
// DllMain —— DLL 入口点
// ============================================================================

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        // 禁用线程通知，减少不必要的 DllMain 调用
        DisableThreadLibraryCalls(hModule);
        // 在独立线程中初始化，避免阻塞加载器锁
        CreateThread(nullptr, 0, InitializeThread, nullptr, 0, nullptr);
        break;

    case DLL_PROCESS_DETACH:
        // DLL 卸载时的清理（可选）
        Hook::ShutdownAllHooks();
        break;

    default:
        break;
    }

    return TRUE;
}
