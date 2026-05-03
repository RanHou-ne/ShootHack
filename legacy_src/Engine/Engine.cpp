// ============================================================
// 文件：engine.cpp
// 模块：IL2CPP 引擎核心实现
// 功能：为 Unity IL2CPP 游戏提供完整的运行时交互能力
//       包括窗口/模块句柄获取、类/方法/字段解析、对象创建与操作、
//       场景管理、字符串转换等常用功能
// 技术要点：
//   - 全部通过动态 GetProcAddress 解析 GameAssembly.dll 导出符号
//   - 使用 X-Macro 技术集中管理 100+ 个 IL2CPP C API，避免硬编码
//   - 所有函数均做严格空指针检查，防止崩溃
//   - 兼容不同 Unity 版本（对应 il2cpp_api.h 中的 API 定义）
// ============================================================

#include "engine.h"
#include <vector>
#include <limits>



// ============================================================
// X-Macro 技术：IL2CPP API 统一声明与加载
// -------------------------------------------------------------
// 原理说明：
//   il2cpp_api.h 中只包含形如 DO_API(RetType, Name, Args) 的宏列表，
//   不包含任何实现代码。通过在不同位置 #define DO_API 后 #include，
//   可以让同一列表“展开”出完全不同的代码：
//     第1次展开 → 全局函数指针变量定义（= NULL）
//     第2次展开 → GetProcAddress 动态赋值语句
//   这种写法极大减少重复劳动和出错概率，新增/删除/修改 API 只需改一个文件。
// ============================================================

// X-macro 第 1 次展开：
// 1) il2cpp_api.h 只是一个 DO_API(...) 的 API 列表。
// 2) 这里重定义 DO_API，使每一行都变成“全局函数指针变量定义”。
// 3) 例如：
//    DO_API(const char*, il2cpp_field_get_name, (FieldInfo* field))
//    会展开成：il2cpp_field_get_name_t il2cpp_field_get_name = NULL;
#define DO_API(RetType, Name, Args) \
    Name##_t Name = NULL; 
#include "il2cpp_api.h" 
#undef DO_API

namespace Engine
{
    // ============================================================
    // 全局运行期缓存
    // -------------------------------------------------------------
    // 这些句柄在 Initialize() 中一次性解析，后续所有函数直接使用，
    // 避免重复调用 FindWindow/GetModuleHandle，提升性能并减少副作用。
    // ============================================================
    HWND hWnd = NULL;
    HMODULE hUnityPlayer = NULL;
    HMODULE hGameAssembly = NULL;

    void Initialize()
    {
        // Unity 在 Windows 下主窗口类名通常是 "UnityWndClass"。
        // 通过 FindWindowA 找到游戏主窗口句柄，后续可用于：
        //   - 注入 DLL 后的窗口消息处理
        //   - 获取客户区大小用于渲染 Hook
        //   - 判断游戏是否处于前台
        hWnd = FindWindowA("UnityWndClass", NULL);
        if (hWnd == NULL)
        {
            printf("[-] 未找到Unity窗口 (UnityWndClass)\n");
        }
        else
        {
            printf("[+] 找到Unity窗口: 0x%p\n", hWnd);
        }

        // UnityPlayer.dll 里包含 D3D11/D3D12 渲染上下文、SwapChain 等核心对象。
        // 可用于后续 Present Hook 拿到真实的 IDXGISwapChain。
        // 注意：部分游戏可能存在多个 UnityPlayer.dll，这里取第一个加载的即可。
        hUnityPlayer = GetModuleHandleA("UnityPlayer.dll");
        if (hUnityPlayer == NULL)
        {
            printf("[-] 未找到UnityPlayer.dll\n");
        }
        else
        {
            printf("[+] 找到UnityPlayer.dll: 0x%p\n", hUnityPlayer);
        }

        // IL2CPP 游戏几乎 100% 由 GameAssembly.dll 导出所有 IL2CPP C API。
        // 这是整个引擎的“心脏”，所有后续 il2cpp_xxx 调用都依赖于此模块句柄。
        hGameAssembly = GetModuleHandleA("GameAssembly.dll");
        if (hGameAssembly == NULL)
        {
            printf("[-] 未找到GameAssembly.dll\n");
        }
        else
        {
            printf("[+] 找到GameAssembly.dll: 0x%p\n", hGameAssembly);
        }

        // X-macro 第 2 次展开：
        // 再次重定义 DO_API，让每个 API 名都通过 GetProcAddress 动态解析。
        // 这一步会给上面定义的所有全局函数指针赋值。
        // 如果某个版本把符号删掉/改名，对应指针会保持 NULL，使用前要判空。
        #define DO_API(RetType, Name, Args) \
            Name = (Name##_t)GetProcAddress(hGameAssembly, #Name); 
            #include "il2cpp_api.h" 
            #undef DO_API

        // 简单校验：打印一个函数指针地址，非空通常说明解析成功。
        printf("[+] IL2CPP API 加载完成: il2cpp_field_get_name = %p\n", il2cpp_field_get_name);
    }

    HWND GetHwnd()
    {
        return hWnd;
    }

    IDXGISwapChain* Engine::GetSwapChain()
    {
        // 现在直接从 PresentHook 获取（通过虚表 Hook 拿到的真实 SwapChain）。
        // 相比硬编码偏移，这种方式不依赖 UnityPlayer 内部固定地址，版本兼容性更好。
        // 注意：PresentHook 必须在 Initialize() 之前或同时完成 Hook。
        return PresentHook::GetSwapChain();
    }

    // ============================================================
    // 使用il2cpp导出函数获取函数指针
    // ============================================================
 

}