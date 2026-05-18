// ============================================================================
// 文件：src/Engine/Engine.cpp
// 模块：引擎层 - IL2CPP 引擎核心实现
// ============================================================================
//
// 【本文件在整体架构中的角色】
//
//   Engine.h 负责「声明」，Engine.cpp 负责「定义和初始化」。
//   它是整个引擎层的"心脏"，做三件核心事情：
//
//   1. 定义所有函数指针变量（初始化为 nullptr）
//   2. 在 Initialize() 中把函数指针绑定到真实地址
//   3. 提供工具函数（GetMethod、GetTypeName 等）
//
// 【X-Macro 在本文件中的三次展开】
//
//   ┌──────────────────┬─────────────────────────────────────────────────────┐
//   │ 展开位置           │ DO_API/DO_FUNC 展开为                                │
//   ├──────────────────┼─────────────────────────────────────────────────────┤
//   │ 第269行（全局作用域）│ X##_t X = nullptr;                                  │
//   │                  │ → 定义全局函数指针变量，初始为空指针                       │
//   ├──────────────────┼─────────────────────────────────────────────────────┤
//   │ 第324行（Initialize │ X = (X##_t)GetProcAddress(g_engineGameAssembly, #X); │
//   │   函数体内）       │ → 运行时从 GameAssembly.dll 获取真实函数地址              │
//   ├──────────────────┼─────────────────────────────────────────────────────┤
//   │ 第334行（Initialize │ X = (X##_t)GetMethod(Assembly, Namespace, Class, #X); │
//   │   函数体内）       │ → 运行时通过 IL2CPP API 查找 C# 方法地址                │
//   └──────────────────┴─────────────────────────────────────────────────────┘
//
// 【Initialize() 完整初始化流程】
//
//   游戏启动后，你的 DLL 被注入，调用 Engine::Initialize()：
//
//   Step 1: FindWindowA("UnityWndClass") → 获取游戏窗口句柄
//   Step 2: GetModuleHandleA("GameAssembly.dll") → 获取 IL2CPP 核心 DLL 句柄
//   Step 3: GetProcAddress 循环绑定所有 il2cpp_xxx 函数指针（官方 C API）
//   Step 4: GetMethod 循环绑定所有游戏 C# 方法函数指针（自定义方法）
//
//   之后就可以直接用 il2cpp_domain_get()、FindObjectsOfType() 等调用了！
//
// 【迁移到其他游戏时怎么办？】
//
//   本文件的核心逻辑（Initialize、GetMethod）对所有 IL2CPP 游戏通用！
//   你只需要：
//     1. 修改 il2cpp_function.h 中的 C# 方法列表
//     2. 如果游戏窗口类名不是 "UnityWndClass"，修改 FindWindowA 参数
//     3. 其他代码基本不用改
//
// ============================================================================

#include "Engine.h"
#include "../Hook/PresentHook.h"   // 用于获取 SwapChain（DX11 Hook）
#include "class.h"                 // Unity_Array 定义
#include "../UI/Menu/MenuSwitches.h" // UI 开关状态
#include "../ImGui/imgui.h"        // ImGui 绘制 API

#include <string>                  // std::string 支持
#include <cstdio>                  // printf
#include <cctype>                  // tolower
#include <algorithm>               // std::remove_if
#include <unordered_set>           // std::unordered_set


// ============================================================================
// X-Macro 展开点 ①：定义所有 IL2CPP 官方 C API 函数指针变量
// ============================================================================
//
// 这里 #define DO_API 让每个 API 生成一个变量定义：
//
//   DO_API(Il2CppDomain*, il2cpp_domain_get, ())
//   展开为：il2cpp_domain_get_t il2cpp_domain_get = nullptr;
//
// 编译后效果：所有 il2cpp_xxx 全局变量被定义，初始值为 nullptr（空指针）。
// 在 Initialize() 中会通过 GetProcAddress 给它们赋真实地址。
//
#define DO_API(RetType, Name, Args) \
    Name##_t Name = nullptr;

#include "il2cpp_api.h"     // ← 展开所有 API 的变量定义

#undef DO_API


// ============================================================================
// X-Macro 展开点 ②：定义所有游戏 C# 方法的函数指针变量
// ============================================================================
//
// 和上面同理，DO_FUNC 多了三个参数（AssemblyName、Namespace、ClassName），
// 但在变量定义阶段这些参数不需要用到，宏展开时会忽略多余的参数。
//
//   DO_FUNC(Unity_Array*, FindObjectsOfType, (void*), "UnityEngine.CoreModule", "UnityEngine", "Object")
//   展开为：FindObjectsOfType_t FindObjectsOfType = nullptr;
//
// 注意：AssemblyName、Namespace、ClassName 参数在定义阶段被"丢弃"了，
// 它们只在 Initialize() 中的绑定阶段（展开点 ③）才被用到。
//
#define DO_FUNC(RetType, Name, Args, AssemblyName, Namespace, ClassName, MethodName) \
    Name##_t Name = nullptr;
#include "il2cpp_function.h"
#undef DO_FUNC


namespace Engine
{
    // ================================================================
    // 全局运行期缓存（静态变量，只初始化一次）
    // ================================================================
    // 避免每次调用都重复 FindWindow / GetModuleHandle，提升性能
    static HWND     g_engineHwnd          = nullptr;  // Unity 主窗口句柄
    static HMODULE  g_engineUnityPlayer   = nullptr;  // UnityPlayer.dll 句柄（含渲染相关）
    static HMODULE  g_engineGameAssembly  = nullptr;  // GameAssembly.dll 句柄（IL2CPP 核心）

    // ================================================================
    // Initialize() —— 引擎初始化（必须在注入后调用一次）
    // ================================================================
    //
    // 【完整初始化链路图】
    //
    //   DLL注入 → DllMain → 调用 Engine::Initialize()
    //                              │
    //                              ├─ 1. FindWindowA → 获取窗口句柄
    //                              │
    //                              ├─ 2. GetModuleHandleA → 获取 DLL 句柄
    //                              │     │
    //                              │     ├─ UnityPlayer.dll    → 渲染相关
    //                              │     └─ GameAssembly.dll   → IL2CPP 核心
    //                              │
    //                              ├─ 3. X-Macro 展开点 ③
    //                              │     #define DO_API → GetProcAddress
    //                              │     #include "il2cpp_api.h"
    //                              │     → 所有 il2cpp_xxx 函数指针被绑定
    //                              │
    //                              └─ 4. X-Macro 展开点 ④
    //                                    #define DO_FUNC → GetMethod
    //                                    #include "il2cpp_function.h"
    //                                    → 所有游戏 C# 方法函数指针被绑定
    //
    void Initialize()
    {
        printf("[*] Engine: 开始初始化...\n");

        // ---- Step 1: 查找 Unity 主窗口 ----
        // Unity 在 Windows 下主窗口的类名固定为 "UnityWndClass"。
        // 找到窗口后可以用于 SetWindowHook、截图等操作。
        g_engineHwnd = FindWindowA("UnityWndClass", nullptr);
        if (g_engineHwnd)
            printf("[+] Engine: 找到 Unity 窗口 0x%p\n", g_engineHwnd);
        else
            printf("[-] Engine: 未找到 UnityWndClass 窗口！\n");

        // ---- Step 2: 获取两个核心 DLL 模块句柄 ----
        //
        // Unity 游戏在 Windows 下有两个核心 DLL：
        //
        //   UnityPlayer.dll:
        //     - Unity 引擎的渲染/音频/输入等底层实现
        //     - 包含 D3D11/D3D12 的 SwapChain 和 Device
        //     - 我们通过 Hook 它的 Present 来实现 ImGui 覆盖层
        //
        //   GameAssembly.dll:
        //     - 所有 C# 代码被 IL2CPP 编译后的结果
        //     - 导出所有 il2cpp_xxx C API 函数
        //     - 包含游戏所有 C# 方法的编译后代码
        //     - 我们通过 GetProcAddress 从中获取函数地址
        //
        g_engineUnityPlayer = GetModuleHandleA("UnityPlayer.dll");
        if (g_engineUnityPlayer)
            printf("[+] Engine: 找到 UnityPlayer.dll 0x%p\n", g_engineUnityPlayer);
        else
            printf("[-] Engine: 未找到 UnityPlayer.dll\n");

        g_engineGameAssembly = GetModuleHandleA("GameAssembly.dll");
        if (g_engineGameAssembly)
            printf("[+] Engine: 找到 GameAssembly.dll 0x%p\n", g_engineGameAssembly);
        else
            printf("[-] Engine: 未找到 GameAssembly.dll\n");

        // ---- Step 3: X-Macro 展开点 ③ —— 绑定所有 IL2CPP 官方 C API ----
        //
        // 这里重新 #define DO_API，让每个 API 通过 GetProcAddress 获取真实地址：
        //
        //   DO_API(Il2CppDomain*, il2cpp_domain_get, ())
        //   展开为：
        //     il2cpp_domain_get = reinterpret_cast<il2cpp_domain_get_t>(
        //         GetProcAddress(g_engineGameAssembly, "il2cpp_domain_get"));
        //
        // GetProcAddress 的原理：
        //   Windows 加载 DLL 时，会把导出函数的名称和地址记录在 PE 头的导出表中。
        //   GetProcAddress 就是在这个表中按名称查找，返回函数的内存地址。
        //   因为 IL2CPP 的导出函数名是固定的（il2cpp_xxx），所以可以直接按名查找。
        //
        // reinterpret_cast 的作用：
        //   GetProcAddress 返回的是通用的 FARPROC（相当于 void(*)()），
        //   我们需要转换成具体的函数指针类型（如 il2cpp_domain_get_t），
        //   reinterpret_cast 就是做这个类型转换的。
        //
        // #Name 中的 # 是什么？
        //   # 是 C 预处理器的「字符串化运算符」，把宏参数变成字符串字面量。
        //   例如 #Name 当 Name=il2cpp_domain_get 时，展开为 "il2cpp_domain_get"。
        //   GetProcAddress 需要字符串参数，所以必须加 #。
        //
        #define DO_API(RetType, Name, Args) \
            Name = reinterpret_cast<Name##_t>(GetProcAddress(g_engineGameAssembly, #Name));

        #include "il2cpp_api.h"     // ← 展开所有 GetProcAddress 绑定

        #undef DO_API

        printf("[+] Engine: IL2CPP API 绑定完成！\n");

        // ---- Step 4: X-Macro 展开点 ④ —— 绑定自定义 C# 方法 ----
        //
        // 这里 #define DO_FUNC，让每个方法通过 GetMethod 获取真实地址：
        //
        //   DO_FUNC(Unity_Array*, FindObjectsOfType, (void*),
        //           "UnityEngine.CoreModule", "UnityEngine", "Object")
        //   展开为：
        //     FindObjectsOfType = reinterpret_cast<FindObjectsOfType_t>(
        //         GetMethod("UnityEngine.CoreModule", "UnityEngine", "Object", "FindObjectsOfType"));
        //
        // GetMethod 和 GetProcAddress 的区别：
        //   GetProcAddress: 按「导出函数名」查找，只能找 il2cpp_xxx 官方 API
        //   GetMethod: 按「程序集+命名空间+类名+方法名」查找，能找任何 C# 方法
        //
        // 为什么游戏 C# 方法不能直接用 GetProcAddress？
        //   因为 C# 方法编译后没有固定的导出名。IL2CPP 会把方法编译成 C++ 函数，
        //   但函数名是混淆过的（如 "Player_Fire_m12345"），且不导出。
        //   所以我们只能通过 il2cpp_xxx API 遍历元数据来定位方法地址。
        //
        #define DO_FUNC(RetType, Name, Args, AssemblyName, Namespace, ClassName, MethodName) \
            Name = reinterpret_cast<Name##_t>(GetMethod(AssemblyName, Namespace, ClassName, MethodName));

        #include "il2cpp_function.h"   // ← 展开所有 GetMethod 绑定

        #undef DO_FUNC

        printf("[+] Engine: 自定义 C# 方法绑定完成！\n");
    }

    // ================================================================
    // GetMainCameraSafe() —— 安全获取主摄像机
    // ================================================================
    // 注意：Camera.get_main() 必须在游戏场景加载后才能返回有效指针。
    //       在 Initialize() 阶段调用会返回 nullptr，因为场景尚未就绪。
    //       应在每帧渲染或用户触发时调用此函数。
    Camera* GetMainCameraSafe()
    {
        if (!GetMainCamera)
        {
            printf("[-] GetMainCameraSafe: 函数指针未绑定！\n");
            return nullptr;
        }

        Camera* pCamera = GetMainCamera();
        if (!pCamera)
        {
            // 场景中还没有主摄像机，这是正常的（游戏尚未加载场景）
            return nullptr;
        }

        return pCamera;
    }

    // ================================================================
    // GetHwnd() —— 返回 Unity 主窗口句柄
    // ================================================================
    HWND GetHwnd()
    {
        return g_engineHwnd;
    }

    // ================================================================
    // GetSwapChain() —— 返回 DX11 SwapChain（用于 ImGui 等渲染 Hook）
    // ================================================================
    IDXGISwapChain* GetSwapChain()
    {
        return PresentHook::GetSwapChain();
    }


    // ================================================================
    // GetTypeName(string) —— 去掉命名空间前缀，只保留类名
    // ================================================================
    // 例如 "UnityEngine.GameObject" → "GameObject"
    std::string GetTypeName(std::string Name)
    {
        size_t Idx = Name.find(".");
        if (Idx != std::string::npos)
        {
            Name = Name.substr(Idx + 1);
        }
        return Name;
    }


    // ================================================================
    // GetTypeName(Il2CppType*) —— 获取类型的可读名称（调试用）
    // ================================================================
    std::string GetTypeName(const Il2CppType* pType)
    {
        if (!pType || !il2cpp_type_get_name)
            return "<unknown>";
        return GetTypeName(il2cpp_type_get_name(pType));
    }


    // ================================================================
    // GetMethod() —— 核心函数：通过程序集+命名空间+类+方法名获取函数指针
    // ================================================================
    //
    // 【这是整个架构最关键的函数！理解它就理解了 IL2CPP 方法调用的原理】
    //
    // 【IL2CPP 运行时的层次结构】
    //
    //   在 IL2CPP 中，所有类型信息都按以下层次组织：
    //
    //   Domain（运行时域，全局唯一）
    //     │
    //     ├── Assembly（程序集，= 一个 .dll 文件）
    //     │     │
    //     │     └── Image（镜像，= 程序集的元数据视图）
    //     │           │
    //     │           └── Class（类，= C# 中的类型定义）
    //     │                 │
    //     │                 ├── Method（方法）
    //     │                 ├── Field（字段）
    //     │                 └── Property（属性）
    //     │
    //     ├── Assembly（另一个程序集）
    //     │     └── ...
    //     └── ...
    //
    //   GetMethod 就是沿着这条链路，从顶层一路找到底层的方法指针：
    //
    //   Domain → Assembly → Image → Class → Method → methodPointer
    //      ↑         ↑         ↑        ↑        ↑          ↑
    //   il2cpp_    il2cpp_   il2cpp_  il2cpp_  il2cpp_    *(void**)
    //   domain_get domain_   assembly class_   class_     pMethod
    //              assembly_ get_image from_   get_method
    //              open              name     from_name
    //
    // 【为什么最后要 *(void**)pMethod？】
    //
    //   MethodInfo 结构体的第一个成员是 methodPointer（函数指针）：
    //
    //     struct MethodInfo {
    //         void* methodPointer;    ← 偏移 0x00，就是我们要的函数地址
    //         Il2CppClass* klass;     ← 偏移 0x08
    //         ... 其他元数据 ...
    //     };
    //
    //   因为我们在 Engine.h 中只做了前向声明（没有完整定义），
    //   所以不能写 pMethod->methodPointer，只能用指针解引用：
    //   *(void**)pMethod = 取 MethodInfo 头部 8 字节 = methodPointer
    //
    // 【调用 C# 方法时需要注意的调用约定】
    //
    //   IL2CPP 编译出的 C++ 方法遵循 __stdcall（Windows x64 下实际是 __fastcall），
    //   但有一个重要规则：
    //
    //   实例方法的第一个参数是 this 指针！
    //   例如 C# 中 obj.GetName()，在 C++ 中是 GetName(obj)
    //
    //   静态方法没有 this 指针，参数就是方法声明的参数。
    //
    //   所以在 il2cpp_function.h 中声明方法时：
    //   - 实例方法：第一个参数写 (void*) 代表 this
    //   - 静态方法：直接写方法参数
    //
    void* GetMethod(const char* AssemblyName,
                    const char* Namespace,
                    const char* ClassName,
                    const char* MethodName)
    {
        // 0. 前置检查：确保核心 IL2CPP API 已经绑定
        if (!il2cpp_domain_get || !il2cpp_domain_assembly_open || !il2cpp_assembly_get_image)
        {
            printf("[-] GetMethod: IL2CPP API 未初始化！\n");
            return nullptr;
        }

        // 1. 获取 IL2CPP 运行时域（Domain）
        //    Domain 是最顶层容器，所有程序集都挂载在它下面
        Il2CppDomain* pDomain = il2cpp_domain_get();

        // 2. 打开指定程序集（Assembly）
        //    程序集 = 编译后的 .dll 文件，如 "UnityEngine.CoreModule"
        //    如果名称错误（拼写错误或该程序集未加载），会返回 nullptr
        const Il2CppAssembly* pAssembly = il2cpp_domain_assembly_open(pDomain, AssemblyName);
        if (!pAssembly)
        {
            printf("[-] GetMethod: 无法打开程序集 %s\n", AssemblyName);
            return nullptr;
        }

        // 3. 获取程序集对应的 Image（镜像）
        //    Image 是 Assembly 的元数据视图，包含类型/方法/字段的定义信息
        const Il2CppImage* pImage = il2cpp_assembly_get_image(pAssembly);

        // 4. 根据命名空间和类名找到 Class
        //    这等价于 C# 中的 Type.GetType("Namespace.ClassName")
        Il2CppClass* pClass = il2cpp_class_from_name(pImage, Namespace, ClassName);
        if (!pClass)
        {
            printf("[-] GetMethod: 找不到类 %s::%s\n", Namespace, ClassName);
            return nullptr;
        }

        // 5. 在类中查找指定方法
        //    参数 -1 表示不限制参数个数（忽略重载区分）
        //    如果方法有重载，可能需要指定精确的参数个数
        const MethodInfo* pMethod = il2cpp_class_get_method_from_name(pClass, MethodName, -1);
        if (!pMethod)
        {
            printf("[-] GetMethod: 找不到方法 %s::%s::%s\n", Namespace, ClassName, MethodName);
            return nullptr;
        }

        // 6. 从 MethodInfo 中提取函数指针
        //    MethodInfo 的第一个字段就是方法入口地址（methodPointer）
        //    因为 MethodInfo 只有前向声明，用 *(void**) 解引用头部 8 字节
        void* methodPtr = *(void**)pMethod;
        printf("[+] GetMethod: 成功找到方法 %s::%s::%s at 0x%p\n", Namespace, ClassName, MethodName, methodPtr);

        return methodPtr;
    }





    // ================================================================
    // TestFindObjectsOfType() —— 测试：调用 FindObjectsOfType 获取游戏对象
    // ================================================================
    //
    // 【调用游戏 C# 方法的两种方式】
    //
    //   方式1（本函数使用）：通过 DO_FUNC 绑定的函数指针直接调用
    //     优点：调用简单，像普通函数一样用
    //     缺点：需要提前知道方法签名，需要在 il2cpp_function.h 中声明
    //
    //   方式2：通过 il2cpp_runtime_invoke 反射调用
    //     优点：不需要提前声明，可以调用任意方法
    //     缺点：调用复杂，需要构造参数数组，性能较低
    //
    // 【本函数的调用链路】
    //
    //   1. il2cpp_domain_get → 获取 Domain
    //   2. il2cpp_domain_assembly_open → 打开 UnityEngine.CoreModule
    //   3. il2cpp_assembly_get_image → 获取 Image
    //   4. il2cpp_class_from_name → 找到 GameObject 类
    //   5. il2cpp_class_get_type → 获取 Il2CppType*
    //   6. il2cpp_type_get_object → 包装成 System.Type 实例（C# 层面的 Type 对象）
    //   7. FindObjectsOfType(typeObj) → 调用绑定的 C# 方法，返回对象数组
    //
    //   【为什么要用 il2cpp_type_get_object？】
    //
    //   FindObjectsOfType 的 C# 签名是：
    //     public static Object[] FindObjectsOfType(Type type)
    //
    //   它需要的是一个 System.Type 实例（C# 对象），而不是 Il2CppType*（C++ 结构）。
    //   il2cpp_type_get_object 的作用就是把 C++ 层的 Il2CppType* 包装成 C# 层的 System.Type。
    //   这是 IL2CPP 中 "C++ 元数据 → C# 反射对象" 的桥梁。
    //
    void TestFindObjectsOfType()
    {
        // 1. 通过 IL2CPP C API 直接获取 GameObject 的 Il2CppClass*
        Il2CppDomain* pDomain = il2cpp_domain_get();
        const Il2CppAssembly* pAssembly = il2cpp_domain_assembly_open(pDomain, "UnityEngine.CoreModule");
        if (!pAssembly)
        {
            printf("[-] TestFindObjectsOfType: 无法打开程序集 UnityEngine.CoreModule\n");
            return;
        }
        const Il2CppImage* pImage = il2cpp_assembly_get_image(pAssembly);
        Il2CppClass* pClass = il2cpp_class_from_name(pImage, "UnityEngine", "GameObject");
        if (!pClass)
        {
            printf("[-] TestFindObjectsOfType: 找不到类 UnityEngine::GameObject\n");
            return;
        }

        // 2. 从 Il2CppClass* 获取 Il2CppType*
        const Il2CppType* pType = il2cpp_class_get_type(pClass);
        if (!pType)
        {
            printf("[-] TestFindObjectsOfType: 无法获取 GameObject 的 Il2CppType\n");
            return;
        }

        // 3. il2cpp_type_get_object 将 Il2CppType* 包装成 C# 的 System.Type 对象
        //    FindObjectsOfType 的 C# 签名需要 System.Type 参数
        Il2CppObject* pTypeObj = il2cpp_type_get_object(pType);
        if (!pTypeObj)
        {
            printf("[-] TestFindObjectsOfType: il2cpp_type_get_object 返回空\n");
            return;
        }

        // 4. 调用 FindObjectsOfType，参数是 System.Type 实例指针
        //    这里用的 FindObjectsOfType 是通过 DO_FUNC 绑定的函数指针
        System_Object_array* ObjArray = reinterpret_cast<System_Object_array*>(FindObjectsOfType(pTypeObj));
        if (ObjArray)
        {
            printf("[*] Engine::TestFindObjectsOfType: 找到 %zu 个对象\n", static_cast<size_t>(ObjArray->max_length));
            for (size_t i = 0; i < static_cast<size_t>(ObjArray->max_length) && i < 10; ++i)
            {
                printf("  [%zu] 0x%p\n", i, ObjArray->m_Items[i]);
            }
        }
        else
        {
            printf("[-] Engine::TestFindObjectsOfType: 未找到任何对象 (ObjArray 为空)\n");
        }
    }

    // ================================================================
    // PrintNeedDataZz() —— 打印 GameManager_PVE 的 EnemyList 地址
    // ================================================================
    void PrintNeedDataZz() // 这个函数是为了演示如何通过 GetMethod 获取游戏 C# 方法，并调用它来获取游戏数据（敌人列表地址）
    {
        // 一行搞定：domain → assembly → image → class → type → System.Type // FindObjectsOfType → 获取 GameManager_PVE 实例 → 读取 PVE_EnimyList 字段
       // 1. 获取 GameManager_PVE 的 System.Type 对象
        Il2CppObject* pTypeObj = GetTypeObject("Assembly-CSharp", "Assets.Scripts", "GameManager_PVE");

        // 2. 检查是否成功获取类型对象
        if (!pTypeObj)
        {
            printf("[-] PrintNeedDataZz: 找不到 GameManager_PVE 的 System.Type\n");
            return;
        }


        // 3. 调用 FindObjectsOfType 获取所有 GameManager_PVE 实例
        Unity_Array* Objects = FindObjectsOfType(pTypeObj);
        if (!Objects || Objects->Count == 0)
        {
            printf("[-] PrintNeedDataZz: 未找到 GameManager_PVE 实例\n");
            return;
        }
        // 4. 输出找到的实例数量
        printf("[*] PrintNeedDataZz: 找到 %zu 个 GameManager_PVE 实例\n", Objects->Count);

        // 5. 取第一个实例（假设只有一个），并读取它的 PVE_EnimyList 字段
        GameManager_PVE* pGame = reinterpret_cast<GameManager_PVE*>(Objects->Objects[0]);
        if (!pGame)
        {
            printf("[-] PrintNeedDataZz: GameManager_PVE 实例指针为空\n");
            return;
        }

        // 6. 输出 GameManager_PVE 实例地址和 PVE_EnimyList 字段地址
        Unity_List* pEnemyList = pGame->PVE_EnimyList;
        printf("[*] GameManager_PVE: 0x%p  |  PVE_EnimyList: 0x%p\n", (void*)pGame, (void*)pEnemyList);


        // 7. 如果 PVE_EnimyList 不为空，继续输出敌人列表中的对象地址
        if (pEnemyList && pEnemyList->Objects && pEnemyList->Count > 0)
        {
            printf("[*] PVE_EnimyList 中有 %d 个敌人对象：\n", pEnemyList->Count);
            int limit = (pEnemyList->Count > 20) ? 20 : pEnemyList->Count;
            for (int i = 0; i < limit; ++i)
            {
                // Objects 是 Unity_Array*，Objects->Objects[i] 才是数组元素
                void* pEnemy = pEnemyList->Objects->Objects[i];
                printf("  [%d] 0x%p\n", i, pEnemy);
            }
        }
        else
        {
            printf("[-] PrintNeedDataZz: PVE_EnimyList 为空或未找到敌人对象\n");
        }

        // 在屏幕左上角绘制地址信息
        ImDrawList* drawList = ImGui::GetForegroundDrawList();
        if (drawList)
        {
            char buf[256];
            sprintf(buf, "GameManager_PVE: 0x%p  |  PVE_EnimyList: 0x%p  |  Count: %d",
                (void*)pGame, (void*)pEnemyList, pEnemyList ? pEnemyList->Count : 0);
            drawList->AddText(ImVec2(10, 10), IM_COL32(0, 255, 0, 255), buf);
        }
    }










    // ================================================================
    // DumpObject() —— 打印对象的类型名 + 所有字段（偏移、名称、类型）
    // ================================================================
    //
    // 【用途】
    //   拿到一个 Il2CppObject* 后，不知道它是什么、有什么字段，
    //   调用此函数可以动态打印出完整的字段列表（偏移+名称+类型），
    //   方便在 CE 中对照分析。
    //
    // 【输出示例】
    //   [*] DumpObject: UnityEngine.GameObject at 0x1A2B3C4D
    //     [+0x0010] m_Name         (System.String*)
    //     [+0x0018] m_Tag          (System.String*)
    //     [+0x0020] m_Layer        (int32_t)
    //     [+0x0028] m_IsActive     (bool)
    //     [+0x0030] m_Transform    (UnityEngine.Transform*)
    //
    void DumpObject(Il2CppObject* pObj)
    {
        if (!pObj)
        {
            printf("[-] DumpObject: 对象指针为空！\n");
            return;
        }

        // 获取对象的类型信息
        Il2CppClass* pClass = il2cpp_object_get_class(pObj);
        if (!pClass)
        {
            printf("[-] DumpObject: 无法获取对象类型！\n");
            return;
        }

        const char* ns      = il2cpp_class_get_namespace(pClass);
        const char* clsName = il2cpp_class_get_name(pClass);
        printf("[*] DumpObject: %s%s%s at 0x%p\n",
               ns ? ns : "", (ns && ns[0]) ? "." : "", clsName, pObj);

        // 遍历当前类 + 所有父类的字段
        Il2CppClass* currentClass = pClass;
        while (currentClass)
        {
            const char* curNs      = il2cpp_class_get_namespace(currentClass);
            const char* curClsName = il2cpp_class_get_name(currentClass);

            // 只在有多层继承时打印父类标记
            if (currentClass != pClass)
                printf("  ── 父类: %s%s%s ──\n",
                       curNs ? curNs : "", (curNs && curNs[0]) ? "." : "", curClsName);

            void* iter = nullptr;
            while (FieldInfo* field = il2cpp_class_get_fields(currentClass, &iter))
            {
                const char*   fieldName  = il2cpp_field_get_name(field);
                size_t        offset     = il2cpp_field_get_offset(field);
                const Il2CppType* fieldType = il2cpp_field_get_type(field);
                const char*   typeName   = fieldType ? il2cpp_type_get_name(fieldType) : "?";

                printf("  [+0x%04zX] %-30s (%s)\n", offset, fieldName, typeName ? typeName : "?");
            }

            // 向上遍历父类
            currentClass = il2cpp_class_get_parent(currentClass);
        }
    }

    

    // ================================================================
    // DumpClass() —— 打印类的完整结构（字段、方法、属性）
    // ================================================================
    //
    // 【用途】
    //   不需要对象实例，只通过 Il2CppClass* 就能查看类的全部成员。
    //   适合在 il2cppDumper 不可用时，直接从运行时获取类型信息。
    //
    void DumpClass(Il2CppClass* pClass)
    {
        if (!pClass)
        {
            printf("[-] DumpClass: 类指针为空！\n");
            return;
        }

        const char* ns      = il2cpp_class_get_namespace(pClass);
        const char* clsName = il2cpp_class_get_name(pClass);
        printf("========================================\n");
        printf("[*] Class: %s%s%s\n", ns ? ns : "", (ns && ns[0]) ? "." : "", clsName);
        printf("========================================\n");

        // 基本信息
        int32_t instanceSize = il2cpp_class_instance_size(pClass);
        printf("  实例大小: %d 字节\n", instanceSize);
        printf("  是否值类型: %s\n", il2cpp_class_is_valuetype(pClass) ? "是" : "否");
        printf("  是否抽象类: %s\n", il2cpp_class_is_abstract(pClass) ? "是" : "否");
        printf("  是否枚举:   %s\n", il2cpp_class_is_enum(pClass) ? "是" : "否");

        // 父类
        Il2CppClass* parent = il2cpp_class_get_parent(pClass);
        if (parent)
        {
            const char* pNs  = il2cpp_class_get_namespace(parent);
            const char* pCls = il2cpp_class_get_name(parent);
            printf("  父类: %s%s%s\n", pNs ? pNs : "", (pNs && pNs[0]) ? "." : "", pCls);
        }

        // ---- 字段 ----
        printf("\n  [字段]\n");
        void* fieldIter = nullptr;
        while (FieldInfo* field = il2cpp_class_get_fields(pClass, &fieldIter))
        {
            const char*      name     = il2cpp_field_get_name(field);
            size_t           offset   = il2cpp_field_get_offset(field);
            const Il2CppType* fType    = il2cpp_field_get_type(field);
            const char*      typeName = fType ? il2cpp_type_get_name(fType) : "?";
            int              flags    = il2cpp_field_get_flags(field);

            // field flags: 0x06 = private, 0x05 = public, 0x10 = static
            const char* access = (flags & 0x0007) == 0x0006 ? "private" :
                                 (flags & 0x0007) == 0x0005 ? "public"   :
                                 (flags & 0x0007) == 0x0004 ? "protected" : "other";
            bool isStatic = (flags & 0x0010) != 0;

            printf("    [+0x%04zX] %-6s %-8s %-30s (%s)\n",
                   offset, isStatic ? "static" : "inst", access, name, typeName ? typeName : "?");
        }

        // ---- 方法 ----
        printf("\n  [方法]\n");
        void* methodIter = nullptr;
        while (const MethodInfo* method = il2cpp_class_get_methods(pClass, &methodIter))
        {
            const char*      name    = il2cpp_method_get_name(method);
            uint32_t         paramCount = il2cpp_method_get_param_count(method);
            bool             isStatic   = !il2cpp_method_is_instance(method);
            const Il2CppType* retType   = il2cpp_method_get_return_type(method);
            const char*      retName   = retType ? il2cpp_type_get_name(retType) : "void";

            printf("    %-6s %s(%d params) -> %s at 0x%p\n",
                   isStatic ? "static" : "inst",
                   name, paramCount,
                   retName ? retName : "void",
                   *(void**)method);
        }

        // ---- 属性 ----
        printf("\n  [属性]\n");
        void* propIter = nullptr;
        while (const PropertyInfo* prop = il2cpp_class_get_properties(pClass, &propIter))
        {
            const char* name = il2cpp_property_get_name(const_cast<PropertyInfo*>(prop));
            const MethodInfo* getter = il2cpp_property_get_get_method(const_cast<PropertyInfo*>(prop));
            const MethodInfo* setter = il2cpp_property_get_set_method(const_cast<PropertyInfo*>(prop));

            printf("    %-30s  %s%s\n", name,
                   getter ? "get" : "   ",
                   setter ? "/set" : "");
        }

        printf("========================================\n\n");
    }

    // ================================================================
    // DumpAssembly() —— 打印指定程序集中所有类的名称
    // ================================================================
    //
    // 【用途】
    //   想知道某个 DLL（如 UnityEngine.CoreModule）里有哪些类，
    //   但不想查 dump.cs 文件时，可以直接从运行时遍历。
    //
    void DumpAssembly(const char* AssemblyName)
    {
        if (!il2cpp_domain_get || !il2cpp_domain_assembly_open || !il2cpp_assembly_get_image)
        {
            printf("[-] DumpAssembly: IL2CPP API 未初始化！\n");
            return;
        }

        Il2CppDomain* pDomain = il2cpp_domain_get();
        const Il2CppAssembly* pAssembly = il2cpp_domain_assembly_open(pDomain, AssemblyName);
        if (!pAssembly)
        {
            printf("[-] DumpAssembly: 无法打开程序集 %s\n", AssemblyName);
            return;
        }

        const Il2CppImage* pImage = il2cpp_assembly_get_image(pAssembly);
        size_t classCount = il2cpp_image_get_class_count(pImage);

        printf("[*] DumpAssembly: %s (%zu 个类)\n", AssemblyName, classCount);
        printf("────────────────────────────────────────\n");

        for (size_t i = 0; i < classCount; i++)
        {
            const Il2CppClass* pClass = il2cpp_image_get_class(pImage, i);
            if (!pClass) continue;

            const char* ns      = il2cpp_class_get_namespace(const_cast<Il2CppClass*>(pClass));
            const char* clsName = il2cpp_class_get_name(const_cast<Il2CppClass*>(pClass));

            if (ns && ns[0])
                printf("  %3zu. %s.%s\n", i, ns, clsName);
            else
                printf("  %3zu. %s\n", i, clsName);
        }
        printf("────────────────────────────────────────\n");
    }



    
    // ================================================================
    // GetClass() —— 获取 Il2CppClass*（用于字段遍历、方法查找等）
    // ================================================================
    Il2CppClass* GetClass(const char* AssemblyName,
                          const char* Namespace,
                          const char* ClassName)
    {
        if (!il2cpp_domain_get || !il2cpp_domain_assembly_open || !il2cpp_assembly_get_image)
            return nullptr;

        Il2CppDomain* pDomain = il2cpp_domain_get();
        const Il2CppAssembly* pAssembly = il2cpp_domain_assembly_open(pDomain, AssemblyName);
        if (!pAssembly) return nullptr;

        const Il2CppImage* pImage = il2cpp_assembly_get_image(pAssembly);
        Il2CppClass* pClass = il2cpp_class_from_name(pImage, Namespace, ClassName);
        return pClass;
    }


    // ================================================================
    // GetTypeObject() —— 获取 Il2CppObject*（C# System.Type 实例）
    // ================================================================
    // 封装 domain→assembly→image→class→type→typeObj 链式调用，
    // 供 FindObjectsOfType 等需要 Type 参数的方法使用。
    Il2CppObject* GetTypeObject(const char* AssemblyName,
                                const char* Namespace,
                                const char* ClassName)
    {
        Il2CppClass* pClass = GetClass(AssemblyName, Namespace, ClassName);
        if (!pClass) return nullptr;

        const Il2CppType* pType = il2cpp_class_get_type(pClass);
        if (!pType) return nullptr;

        Il2CppObject* pTypeObj = il2cpp_type_get_object(pType);
        return pTypeObj;
    }


    // ================================================================
    // ThreadGuard —— RAII 自动管理 IL2CPP 线程 attach/detach
    // ================================================================
    ThreadGuard::ThreadGuard()
        : m_pThread(nullptr)
    {
        if (!il2cpp_domain_get || !il2cpp_thread_attach)
            return;
        Il2CppDomain* pDomain = il2cpp_domain_get();
        if (pDomain)
            m_pThread = il2cpp_thread_attach(pDomain);
    }

    ThreadGuard::~ThreadGuard()
    {
        if (m_pThread && il2cpp_thread_detach)
            il2cpp_thread_detach(m_pThread);
    }


    // ================================================================
    // GameObject 信息读取函数
    // ================================================================
    //
    // 【为什么需要这些函数？】
    //
    //   GameObject 是"薄包装器"，C# 托管侧只有 m_CachedPtr（24字节），
    //   真实数据（名称、位置、旋转等）都在 C++ 原生侧。
    //
    //   两种访问方式：
    //     1. 硬编码 C++ 原生偏移 → 快但脆弱，Unity 更新就失效
    //     2. 调用 C# 方法指针 → 稳定跨版本，推荐方式
    //
    //   以下函数全部采用方式2，通过 il2cpp_function.h 中绑定的方法指针调用。

    // ---- GetGameObjectName ----
    // 获取 GameObject 的名称
    //
    // 【实现思路】
    //   C# 中 GameObject 没有直接的 name 属性，name 继承自 UnityEngine.Object。
    //   UnityEngine.Object.get_name() 返回 C# string (Il2CppString*)，
    //   我们通过 il2cpp_string_chars + il2cpp_string_length 转换为 std::string。
    //
    //   但 Object.get_name 未在 il2cpp_function.h 中绑定，
    //   所以这里用 il2cpp_runtime_invoke 反射调用。
    //
    //   也可以通过 m_CachedPtr 读 C++ 原生侧的名称，但偏移因版本而异。
    std::string GetGameObjectName(void* pGameObject)
    {
        if (!pGameObject) return "<null>";

        // 方案：通过 il2cpp_runtime_invoke 反射调用 Object.get_name()
        Il2CppClass* pClass = il2cpp_object_get_class(reinterpret_cast<Il2CppObject*>(pGameObject));
        if (!pClass) return "<unknown>";

        // 向上遍历找到 UnityEngine.Object 类（name 方法定义在这里）
        Il2CppClass* objClass = pClass;
        while (objClass)
        {
            const char* name = il2cpp_class_get_name(objClass);
            if (name && strcmp(name, "Object") == 0) break;
            objClass = il2cpp_class_get_parent(objClass);
        }

        if (objClass)
        {
            const MethodInfo* nameMethod = il2cpp_class_get_method_from_name(objClass, "get_name", 0);
            if (nameMethod && il2cpp_runtime_invoke)
            {
                Il2CppException* exc = nullptr;
                Il2CppObject* result = il2cpp_runtime_invoke(nameMethod, pGameObject, nullptr, &exc);
                if (result && !exc)
                {
                    Il2CppString* str = reinterpret_cast<Il2CppString*>(result);
                    if (il2cpp_string_length && il2cpp_string_chars)
                    {
                        int32_t len = il2cpp_string_length(str);
                        const Il2CppChar* chars = il2cpp_string_chars(str);
                        // Il2CppChar = uint16_t (UTF-16)，简单转 ASCII
                        std::string name;
                        for (int32_t i = 0; i < len && chars[i]; i++)
                        {
                            name += static_cast<char>(chars[i]);
                        }
                        return name;
                    }
                }
            }
        }

        return "<unknown>";
    }

    // ---- GetTransform ----
    // 获取 GameObject 的 Transform 组件
    void* GetTransform(void* pGameObject) //这个返回是void*，代表Transform*
    {
        if (!pGameObject || !GameObject_get_transform) return nullptr; // 先检查指针和函数指针是否有效
        return GameObject_get_transform(pGameObject); // 调用绑定的 C# 方法指针获取 Transform 组件
    }

    // ---- GetPosition ----
    // 获取 Transform 的世界坐标
    Unity_Vector3 GetPosition(void* pTransform) // 这个返回是Unity_Vector3，代表Vector3
    {
        if (!pTransform || !Transform_get_position) // 先检查指针和函数指针是否有效
            return { 0, 0, 0 }; // 如果无效，返回默认坐标 (0, 0, 0)
        return Transform_get_position(pTransform); // 调用绑定的 C# 方法指针获取位置
    }

    // ---- GetRotation ----
    // 获取 Transform 的世界旋转
    Unity_Quaternion GetRotation(void* pTransform)
    {
        if (!pTransform || !Transform_get_rotation)
            return { 0, 0, 0, 1 };
        return Transform_get_rotation(pTransform);
    }




    // ---- GetLossyScale ----
    // 获取 Transform 的全局缩放
    Unity_Vector3 GetLossyScale(void* pTransform)
    {
        if (!pTransform || !Transform_get_lossyScale)
            return { 1, 1, 1 };
        return Transform_get_lossyScale(pTransform);
    }



    // ---- PrintGameObjectInfo ----
    // 打印 GameObject 的完整信息
    void PrintGameObjectInfo(void* pGameObject)
    {
        if (!pGameObject)
        {
            printf("[-] PrintGameObjectInfo: 对象指针为空\n");
            return;
        }

        // 名称
        std::string name = GetGameObjectName(pGameObject);

        // 层级
        int layer = -1;
        if (GameObject_get_layer)
            layer = GameObject_get_layer(pGameObject);

        // 激活状态
        bool activeSelf = false;
        bool activeInHierarchy = false;
        if (GameObject_get_activeSelf)
            activeSelf = GameObject_get_activeSelf(pGameObject);
        if (GameObject_get_activeInHierarchy)
            activeInHierarchy = GameObject_get_activeInHierarchy(pGameObject);

        // Transform
        void* transform = GetTransform(pGameObject);
        Unity_Vector3 pos = { 0, 0, 0 };
        Unity_Vector3 scale = { 1, 1, 1 };
        if (transform)
        {
            pos = GetPosition(transform);
            scale = GetLossyScale(transform);
        }

        // 打印
        printf("[GameObject] \"%s\"  Layer: %d  Active: %s (Hierarchy: %s)\n",
               name.c_str(), layer,
               activeSelf ? "Yes" : "No",
               activeInHierarchy ? "Yes" : "No");
        if (transform)
        {
            printf("  Position: (%.2f, %.2f, %.2f)\n", pos.x, pos.y, pos.z);
            printf("  Scale:    (%.2f, %.2f, %.2f)\n", scale.x, scale.y, scale.z);
        }
        else
        {
            printf("  Transform: <null>\n");
        }
    }

    // ================================================================
    // ================================================================
    //
    //       GOM（GameObjectManager）遍历系统
    //
    // ================================================================
    // ================================================================
    //
    //
    // 【什么是 GOM？】
    //
    //   GOM = GameObjectManager，是 Unity 引擎内部的一个核心管理器。
    //   它维护着「当前场景中所有活跃的游戏对象」的完整列表。
    //
    //   当你在 Unity 编辑器中拖入一个 Cube、Camera、Light 时，
    //   每一个都会被注册到 GOM 中。
    //   当你 Destroy 一个对象时，GOM 会把它从列表中移除。
    //
    //
    // 【GOM 的数据结构 —— 双向循环链表】
    //
    //   GOM 内部使用「双向循环链表」来管理所有对象：
    //
    //   "双向" = 每个节点都有 prev（前一个）和 next（后一个）指针
    //   "循环" = 最后一个节点的 next 指回第一个，第一个的 prev 指回最后一个
    //   "链表" = 节点在内存中不连续，靠指针串联
    //
    //   示意图：
    //
    //            ┌──────────────────────────────────────────────┐
    //            │                                              │
    //            ▼                                              │
    //        ┌────────┐    ┌────────┐    ┌────────┐    ┌────────┐
    //        │GOM头节点│───▶│节点 1  │───▶│节点 2  │───▶│节点 3  │──┐
    //        │        │◀───│        │◀───│        │◀───│        │  │
    //        └────────┘    └────────┘    └────────┘    └────────┘  │
    //            ▲                                                  │
    //            │          (往更多节点...)                          │
    //            │                                                  │
    //            └──────────────────────────────────────────────────┘
    //
    //
    // 【内存中的具体布局】
    //
    //   整个访问链路分为 4 层：
    //
    //   ┌───────────────────────────────────────────────────────────────┐
    //   │ 第1层：UnityPlayer.dll 模块                                  │
    //   │                                                               │
    //   │   UnityPlayer.dll 是 Unity 引擎的核心 DLL，                   │
    //   │   在固定偏移处存储着 GOM 头节点的地址。                        │
    //   │   你通过逆向找到的 0x17C3F18 就是这个偏移。                    │
    //   │                                                               │
    //   │   UnityPlayer.dll 基址 + 0x17C3F18  →  读出一个 8 字节地址    │
    //   │   这个地址就是 GOM 头节点的内存地址                            │
    //   ├───────────────────────────────────────────────────────────────┤
    //   │ 第2层：GOM 头节点                                             │
    //   │                                                               │
    //   │   偏移    大小    含义                                         │
    //   │   0x00    8字节   prev → 指向链表中最后一个节点                │
    //   │   0x08    8字节   next → 指向链表中第一个实际节点              │
    //   │                                                               │
    //   │   GOM 头节点本身不是游戏对象，它只是一个"哨兵"节点，           │
    //   │   用来标记链表的起点和终点。                                   │
    //   │                                                               │
    //   │   我们从它的 next（+0x08）出发，开始遍历。                     │
    //   ├───────────────────────────────────────────────────────────────┤
    //   │ 第3层：ObjectListNode（链表节点）                             │
    //   │                                                               │
    //   │   偏移    大小    含义                                         │
    //   │   0x00    8字节   prev → 上一个节点                           │
    //   │   0x08    8字节   next → 下一个节点                           │
    //   │   0x10    8字节   BaseObject* → 指向真正的游戏对象数据        │
    //   │                                                               │
    //   │   每个活跃的 GameObject 都对应一个这样的节点。                 │
    //   │   遍历就是沿着 next 指针一直走，直到回到 GOM 头节点。         │
    //   ├───────────────────────────────────────────────────────────────┤
    //   │ 第4层：BaseObject（真正的游戏对象）                           │
    //   │                                                               │
    //   │   这是 Unity 引擎内部的原生 C++ 对象（不是 C# 托管对象），    │
    //   │   包含了游戏对象的各种数据：名称、组件列表、层级等。          │
    //   │                                                               │
    //   │   偏移    含义                                                │
    //   │   ...     其他字段（因 Unity 版本而异）                       │
    //   │   +XX     m_Name → 指向对象名称的 C 字符串（偏移不固定！）    │
    //   │   ...     其他字段                                            │
    //   │                                                               │
    //   │   ★ m_Name 的偏移因 Unity 版本不同而不同，                    │
    //   │     我们通过「自动扫描」来找到它。                             │
    //   └───────────────────────────────────────────────────────────────┘
    //
    //
    // 【完整遍历流程图】
    //
    //   ┌──────────────────────────────────────────────────────────────────┐
    //   │                                                                  │
    //   │   Step 1: GetModuleHandleA("UnityPlayer.dll")                    │
    //   │           获取 UnityPlayer.dll 在内存中的加载基址                │
    //   │                           │                                      │
    //   │                           ▼                                      │
    //   │   Step 2: 读取 [基址 + 0x17C3F18]                               │
    //   │           得到 GOM 头节点地址                                    │
    //   │                           │                                      │
    //   │                           ▼                                      │
    //   │   Step 3: 读取 [GOM头 + 0x08]  (头节点的 next)                  │
    //   │           得到第一个 ObjectListNode 地址                         │
    //   │                           │                                      │
    //   │                           ▼                                      │
    //   │   Step 4: 读取 [节点 + 0x10]  (节点的 BaseObject 指针)          │
    //   │           得到第一个游戏对象地址                                 │
    //   │                           │                                      │
    //   │                           ▼                                      │
    //   │   Step 5: 在对象内存中扫描，找到 m_Name 字段偏移                │
    //   │           (自动尝试常见偏移 + 全扫描)                            │
    //   │                           │                                      │
    //   │                           ▼                                      │
    //   │   Step 6: 开始循环遍历 ───────────────────────────────────┐      │
    //   │           │                                                │      │
    //   │           │   ┌────────────────────────────────────────┐  │      │
    //   │           │   │  当前节点 == GOM头？                    │  │      │
    //   │           │   │  是 → 遍历结束（绕了一圈回来）         │  │      │
    //   │           │   │  否 → 继续                             │  │      │
    //   │           │   └────────────┬───────────────────────────┘  │      │
    //   │           │                │                               │      │
    //   │           │                ▼                               │      │
    //   │           │   ┌────────────────────────────────────────┐  │      │
    //   │           │   │  读取 [节点 + 0x10] → BaseObject       │  │      │
    //   │           │   │  读取 [BaseObject + m_Name偏移] → 名称 │  │      │
    //   │           │   │  输出：序号、节点地址、对象地址、名称   │  │      │
    //   │           │   └────────────┬───────────────────────────┘  │      │
    //   │           │                │                               │      │
    //   │           │                ▼                               │      │
    //   │           │   ┌────────────────────────────────────────┐  │      │
    //   │           │   │  读取 [节点 + 0x08] → next 节点        │  │      │
    //   │           │   │  跳转到下一个节点，继续循环             │  │      │
    //   │           │   └────────────────────────────────────────┘  │      │
    //   │           │                                                │      │
    //   │           └────────────────────────────────────────────────┘      │
    //   │                                                                  │
    //   └──────────────────────────────────────────────────────────────────┘
    //
    //
    // 【为什么 m_Name 偏移不确定？】
    //
    //   Unity 的 BaseObject 是 C++ 原生对象，不是 C# 托管对象。
    //   它的内部结构（字段排列顺序、对齐方式）会随 Unity 版本变化：
    //
    //     Unity 2019.4:  m_Name 可能在 +0x60
    //     Unity 2020.3:  m_Name 可能在 +0x68
    //     Unity 2021.x:  m_Name 可能在 +0x48
    //     Unity 2022.x:  m_Name 可能在 +0x30
    //     ...等等，每个版本都不一样
    //
    //   所以我们不能硬编码偏移，需要「自动扫描」。
    //
    //
    // 【自动扫描 m_Name 偏移的原理】
    //
    //   m_Name 是一个 char* 指针，指向一个可读的 ASCII 字符串（如 "Main Camera"）。
    //   我们对第一个对象的内存，从偏移 0x10 开始，每 8 字节取一个值，
    //   尝试把它当作 char* 指针，看指向的内容是否是可打印的 ASCII 字符串。
    //
    //   举例：假设 BaseObject 地址 = 0x12340000
    //
    //     偏移 0x10: 读出值 0x0000000000000001  → 不像字符串地址，跳过
    //     偏移 0x18: 读出值 0x000000000000FF00  → 不像字符串地址，跳过
    //     偏移 0x68: 读出值 0x000001D540ABC000  → 读这个地址 → "Main Camera" ✓
    //                                                      → 这就是 m_Name 偏移！
    //
    //
    // 【SEH 保护 —— 为什么要 try/except？】
    //
    //   在读取游戏内存时，任何指针都可能是无效的（已释放、未映射等）。
    //   如果直接读一个无效地址，程序会崩溃（Access Violation）。
    //   SEH（Structured Exception Handling）是 Windows 的异常处理机制，
    //   用 __try/__except 包裹后，即使读到无效地址也不会崩溃，
    //   而是跳到 __except 块中返回一个错误值。
    //
    //
    // 【和 IL2CPP 的区别】
    //
    //   你项目中已有的 FindObjectsOfType 是通过 IL2CPP API 获取 C# 对象，
    //   而 GOM 遍历是读 Unity 引擎的原生 C++ 层。两者不同：
    //
    //     ┌────────────────────┬──────────────────────────────────────┐
    //     │                    │ FindObjectsOfType      │ GOM 遍历     │
    //     ├────────────────────┼──────────────────────────────────────┤
    //     │ 层次               │ C# 托管层              │ C++ 原生层   │
    //     │ 获取方式           │ IL2CPP API 反射调用     │ 内存直接读取 │
    //     │ 返回类型           │ Il2CppObject* (C#对象)  │ 原生指针     │
    //     │ 能获取的对象       │ 只能获取已知类型的实例  │ 所有活跃对象 │
    //     │ 名称获取           │ Object.get_name() 方法  │ 内存偏移扫描 │
    //     │ 性能               │ 较慢（需要反射调用）    │ 较快（纯指针）│
    //     │ 稳定性             │ 高（API 不变）          │ 中（偏移会变）│
    //     └────────────────────┴──────────────────────────────────────┘
    //
    //
    // 【参数说明】
    //   offset   — GOM 在 UnityPlayer.dll 中的偏移，默认 0x17C3F18
    //   maxCount — 最大遍历数量，0 表示不限制（遍历全部）
    //
    // ================================================================



    // ================================================================
    //  辅助函数 ①：SafeReadString —— 安全读取 C 字符串
    // ================================================================
    //
    // 【功能】
    //   从给定的内存地址读取一个以 '\0' 结尾的 ASCII 字符串。
    //   如果地址无效或内容不是可打印 ASCII，安全返回 false，不会崩溃。
    //
    // 【为什么需要这个函数？】
    //   当我们扫描 m_Name 偏移时，会尝试把很多指针当作字符串来读。
    //   其中大部分指针是无效的，直接读会崩溃。
    //   所以必须用 SEH (__try/__except) 保护，遇到异常就返回 false。
    //
    // 【参数】
    //   strPtr  — 要读取的内存地址（假设它指向一个 char*）
    //   outBuf  — 输出缓冲区，读到的字符串会拷贝到这里
    //   bufSize — 输出缓冲区大小
    //
    // 【返回值】
    //   true  = 成功读到一个有效的 ASCII 字符串
    //   false = 地址无效 / 内容不是可打印 ASCII / 读取异常
    //
    // ================================================================
    static bool SafeReadString(uintptr_t strPtr, char* outBuf, size_t bufSize)
    {
        // ---- 前置检查：参数不能为空 ----
        if (!strPtr || !outBuf || bufSize == 0) return false;

        __try  // SEH 保护：如果下面的内存访问触发异常，不会崩溃
        {
            const char* p = reinterpret_cast<const char*>(strPtr);

            // ---- 逐字节检查是否是有效的可打印 ASCII 字符串 ----
            size_t i = 0;
            for (; i < bufSize - 1; ++i)
            {
                char c = p[i];

                // 遇到字符串结束符，正常结束
                if (c == '\0') break;

                // 如果遇到非可打印字符（< 0x20 或 > 0x7E），
                // 说明这不是一个正常的 ASCII 字符串，返回 false
                if ((unsigned char)c < 0x20 || (unsigned char)c > 0x7E)
                    return false;
            }

            // 空字符串也不算有效名称
            if (i == 0) return false;

            // 拷贝到输出缓冲区并加上结束符
            memcpy(outBuf, p, i);
            outBuf[i] = '\0';
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)  // 捕获所有异常（包括 Access Violation）
        {
            return false;  // 地址不可读，安全返回
        }
    }



    // ================================================================
    //  辅助函数 ②：HexDump —— 内存十六进制转储
    // ================================================================
    //
    // 【功能】
    //   把一段内存以「十六进制 + ASCII」的形式打印到控制台，
    //   类似于调试器（如 x64dbg）中常见的内存查看窗口。
    //
    // 【输出示例】
    //     0000: 01 00 00 00 48 65 6C 6C 6F 00 FF 00 00 00 00 00 |.Hello.......|
    //     0010: 00 00 00 00 00 00 00 00 A0 3F 00 01 D5 40 AB C0 |.........?...@..|
    //
    // 【用途】
    //   当自动扫描找不到 m_Name 偏移时，人工查看 BaseObject 的原始内存，
    //   通过观察哪个偏移位置存着一个指针（8字节，指向 ASCII 字符串），
    //   就能手动确定 m_Name 的偏移。
    //
    // 【参数】
    //   address — 要转储的起始地址
    //   size    — 转储多少字节
    //
    // ================================================================
    static void HexDump(uintptr_t address, size_t size)
    {
        const uint8_t* data = reinterpret_cast<const uint8_t*>(address);

        for (size_t i = 0; i < size; i += 16)  // 每行显示 16 字节
        {
            // ---- 打印行偏移 ----
            printf("  %04zX: ", i);

            // ---- 打印十六进制值 ----
            for (size_t j = 0; j < 16 && (i + j) < size; ++j)
            {
                __try
                {
                    printf("%02X ", data[i + j]);
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    printf("?? ");  // 地址不可读，显示 ??
                }
            }

            // ---- 不足 16 字节时补空格对齐 ----
            for (size_t j = (size - i < 16 ? size - i : 16); j < 16; ++j)
                printf("   ");

            // ---- 打印 ASCII 解读 ----
            // 可打印字符直接显示，不可打印的显示为 '.'
            printf(" |");
            for (size_t j = 0; j < 16 && (i + j) < size; ++j)
            {
                __try
                {
                    uint8_t c = data[i + j];
                    printf("%c", (c >= 0x20 && c <= 0x7E) ? c : '.');
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    printf("?");
                }
            }
            printf("|\n");
        }
    }



    // ================================================================
    //  辅助函数 ③：ScanForNameOffset —— 自动扫描 m_Name 偏移
    // ================================================================
    //
    // 【功能】
    //   在 BaseObject 的内存中，自动找到 m_Name 字段的偏移量。
    //
    // 【原理】
    //   m_Name 是一个 char* 指针，指向一个可读的字符串（如 "Main Camera"）。
    //   我们从偏移 0x10 开始，每 8 字节取一个值（64位下指针大小 = 8字节），
    //   把它当作 char*，尝试用 SafeReadString 读取。
    //   如果读到了有效的 ASCII 字符串，那这个偏移就是 m_Name 的位置。
    //
    // 【扫描策略】
    //   为了提高速度，采用两阶段策略：
    //
    //   阶段1: 优先尝试 10 个常见偏移（0x30, 0x38, 0x40, 0x48, 0x50,
    //          0x58, 0x60, 0x68, 0x70, 0x78）
    //          这些是 Unity 各版本中最常出现的 m_Name 位置
    //
    //   阶段2: 如果常见偏移都没命中，从 0x10 到 0x100 全扫描，
    //          每 8 字节尝试一次，跳过阶段1已尝试过的
    //
    // 【举例】
    //   假设 BaseObject = 0x12340000，m_Name 在偏移 0x68：
    //
    //   尝试偏移 0x60:
    //     读取 [0x12340060] 得到 0x0000000000000001
    //     SafeReadString(0x0000000000000001) → 不是有效字符串 → 跳过
    //
    //   尝试偏移 0x68:
    //     读取 [0x12340068] 得到 0x000001D540ABC000
    //     SafeReadString(0x000001D540ABC000) → "Main Camera" → 找到！
    //     返回 0x68
    //
    // 【返回值】
    //   >= 0 : 找到的偏移量
    //   -1   : 未找到（可能对象没有名称，或者字段布局完全不同）
    //
    // 【参数】
    //   baseObject — BaseObject 的内存地址
    //   outName    — 如果找到，把名称拷贝到这里
    //   nameBufSize — outName 缓冲区大小
    //
    // ================================================================
    static int ScanForNameOffset(uintptr_t baseObject, char* outName, size_t nameBufSize)
    {
        // ---- 阶段1：优先尝试常见偏移 ----
        // 这些是 Unity 2019~2023 各版本中 m_Name 最常出现的位置
        static const int commonOffsets[] = {
            0x60, 0x68, 0x48, 0x50, 0x58, 0x30, 0x38, 0x40, 0x70, 0x78
        };

        for (int off : commonOffsets)
        {
            // 读取 [BaseObject + off] 处的 8 字节，当作指针
            uintptr_t ptr = *reinterpret_cast<uintptr_t*>(baseObject + off);

            // 如果这个指针指向一个有效的 ASCII 字符串，就找到了！
            if (ptr && SafeReadString(ptr, outName, nameBufSize))
            {
                return off;
            }
        }

        // ---- 阶段2：全扫描（0x10 ~ 0x100，每 8 字节）----
        for (int off = 0x10; off < 0x100; off += 8)
        {
            // 跳过阶段1已经尝试过的偏移
            bool skip = false;
            for (int co : commonOffsets) { if (off == co) { skip = true; break; } }
            if (skip) continue;

            uintptr_t ptr = *reinterpret_cast<uintptr_t*>(baseObject + off);
            if (ptr && SafeReadString(ptr, outName, nameBufSize))
            {
                return off;
            }
        }

        return -1;  // 全部扫描完毕，未找到
    }



    // ================================================================
    //  核心函数：TraverseGOM —— 遍历 GameObjectManager 链表
    // ================================================================
    //
    // 【请先阅读文件顶部的 GOM 总览注释，了解数据结构和遍历流程】
    //
    // 【参数】
    //   offset   — GOM 在 UnityPlayer.dll 中的偏移（默认 0x17C3F18）
    //   maxCount — 最大遍历数量（0 = 不限制，遍历全部）
    //
    // ================================================================
    void TraverseGOM(uintptr_t offset, size_t maxCount)
    {
        // ================================================================
        //  Step 1: 获取 UnityPlayer.dll 的模块基址
        // ================================================================
        //
        //  GetModuleHandleA 返回已加载 DLL 在内存中的基址（开头地址）。
        //  因为 DLL 已经被游戏加载了，所以不需要 LoadLibrary，直接取句柄。
        //
        //  例如：UnityPlayer.dll 可能加载在 0x00007FFE12340000
        //        之后所有偏移都是相对于这个基址的
        //
        printf("\n============ GOM 遍历开始 ============\n");

        HMODULE hUnityPlayer = GetModuleHandleA("UnityPlayer.dll");
        if (!hUnityPlayer)
        {
            printf("[-] TraverseGOM: 未找到 UnityPlayer.dll\n");
            return;
        }
        printf("[*] UnityPlayer.dll 基址: 0x%p\n", hUnityPlayer);


        // ================================================================
        //  Step 2: 计算 GOM 头节点的地址
        // ================================================================
        //
        //  GOM 头节点指针存储在 UnityPlayer.dll 的固定偏移处。
        //  我们用「基址 + 0x17C3F18」算出存储地址，然后读取其中的值。
        //
        //  内存示意：
        //
        //    UnityPlayer.dll 基址 = 0x00007FFE12340000（举例）
        //
        //    地址 0x00007FFE13B03F18 (= 基址+0x17C3F18)
        //    存着的值 = 0x0000023456780010 ← 这就是 GOM 头节点的地址
        //
        //    我们要做的就是：
        //      1. 算出 0x00007FFE13B03F18
        //      2. 从这个地址读取 8 字节，得到 0x0000023456780010
        //      3. 0x0000023456780010 就是 GOM 头节点在内存中的位置
        //
        uintptr_t* gomPtrAddr = reinterpret_cast<uintptr_t*>(
            reinterpret_cast<uintptr_t>(hUnityPlayer) + offset);

        uintptr_t gomHead = *gomPtrAddr;
        if (!gomHead)
        {
            printf("[-] TraverseGOM: GOM 头节点指针为空 (地址 0x%p)\n", gomPtrAddr);
            return;
        }
        printf("[*] GOM 头节点地址: 0x%p\n", (void*)gomHead);
        printf("[*] GOM 指针存储地址: 0x%p (UnityPlayer + 0x%llX)\n\n",
               gomPtrAddr, static_cast<unsigned long long>(offset));


        // ================================================================
        //  Step 3: 从 GOM 头节点读取第一个链表节点
        // ================================================================
        //
        //  GOM 头节点结构（8 字节对齐的指针）：
        //
        //    GOM 头节点地址
        //    │
        //    ├── +0x00 (prev) → 指向链表中最后一个节点
        //    │
        //    └── +0x08 (next) → 指向链表中第一个实际游戏对象节点
        //                         ↑
        //                         我们要读的就是这个！
        //
        //  内存示意：
        //
        //    [gomHead + 0x00] = 0x0000023456780FF0  (prev = 最后一个节点)
        //    [gomHead + 0x08] = 0x0000023456780050  (next = 第一个节点) ← 读这个
        //
        uintptr_t firstNode = *reinterpret_cast<uintptr_t*>(gomHead + 0x08);
        if (!firstNode)
        {
            printf("[-] TraverseGOM: 第一个节点指针为空\n");
            return;
        }


        // ================================================================
        //  Step 4: 读取第一个节点中的 BaseObject，并做 Hex Dump
        // ================================================================
        //
        //  每个 ObjectListNode 结构：
        //
        //    节点地址
        //    │
        //    ├── +0x00 (prev) → 上一个节点
        //    │
        //    ├── +0x08 (next) → 下一个节点
        //    │
        //    └── +0x10 (BaseObject*) → 指向真正的游戏对象
        //                                ↑
        //                                我们要读的就是这个！
        //
        //  BaseObject 是 Unity 引擎的原生 C++ 对象，内存布局：
        //
        //    BaseObject 地址
        //    │
        //    ├── +0x00 ~ +0x0F  头部数据（类指针、引用计数等）
        //    ├── +0x10 ~ +0x17  其他字段
        //    ├── ...
        //    ├── +XX            m_Name (char*) → 指向名称字符串
        //    │                    ↑ 这个偏移在每个 Unity 版本都不同！
        //    ├── ...
        //    └── +0xFF          扫描范围边界
        //
        //  我们对第一个对象做 Hex Dump（256字节），这样你可以
        //  在控制台看到原始内存数据，辅助确认 m_Name 偏移。
        //
        uintptr_t firstObject = *reinterpret_cast<uintptr_t*>(firstNode + 0x10);

        // 存储自动检测到的 m_Name 偏移，-1 表示未找到
        int detectedNameOffset = -1;

        if (firstObject)
        {
            printf("[*] 第一个 BaseObject 地址: 0x%p\n", (void*)firstObject);

            // ---- Hex Dump：把 BaseObject 前 256 字节以十六进制 + ASCII 方式打印 ----
            // 输出示例：
            //   0000: 48 01 00 00 00 00 00 00 A0 3F 00 01 D5 40 00 00 |H........?...@..|
            //   0010: ...
            // 这样你可以人工查看哪些位置可能是 m_Name 指针
            printf("[*] BaseObject 内存 Hex Dump (前 256 字节):\n");
            HexDump(firstObject, 256);

            // ---- 自动扫描 m_Name 偏移 ----
            // 尝试从 0x10 到 0x100 的各种偏移，看哪个位置的值
            // 是一个指向 ASCII 字符串的有效指针
            char nameBuf[128] = {};
            int nameOffset = ScanForNameOffset(firstObject, nameBuf, sizeof(nameBuf));
            if (nameOffset >= 0)
            {
                printf("\n[+] 自动发现 m_Name 偏移: +0x%X  (值: \"%s\")\n", nameOffset, nameBuf);
                detectedNameOffset = nameOffset;  // 保存到外部变量，后面遍历时用
            }
            else
            {
                printf("\n[-] 自动扫描未找到有效名称偏移\n");
                printf("[*] 请检查上方 Hex Dump，手动确定 m_Name 的偏移位置\n");
                printf("[*] 尝试方法：找到 Hex Dump 中指向 ASCII 字符串的 8 字节指针\n");
                printf("[*] 它相对于 BaseObject 的偏移就是 m_Name 的偏移\n");
            }
            printf("\n");
        }


        // ================================================================
        //  Step 5: 遍历双向循环链表，输出所有游戏对象
        // ================================================================
        //
        //  遍历过程示意（假设链表有 3 个实际节点）：
        //
        //    初始：currentNode = firstNode (节点1)
        //
        //    循环 1：
        //      currentNode = 节点1  (不等于 gomHead，继续)
        //      读取 [节点1 + 0x10] → BaseObject_A
        //      读取 [BaseObject_A + m_Name偏移] → "Main Camera"
        //      输出: [0] 节点1地址  BaseObject_A地址  Main Camera
        //      读取 [节点1 + 0x08] → 节点2
        //      currentNode = 节点2
        //
        //    循环 2：
        //      currentNode = 节点2  (不等于 gomHead，继续)
        //      读取 [节点2 + 0x10] → BaseObject_B
        //      读取 [BaseObject_B + m_Name偏移] → "Player"
        //      输出: [1] 节点2地址  BaseObject_B地址  Player
        //      读取 [节点2 + 0x08] → 节点3
        //      currentNode = 节点3
        //
        //    循环 3：
        //      currentNode = 节点3  (不等于 gomHead，继续)
        //      读取 [节点3 + 0x10] → BaseObject_C
        //      读取 [BaseObject_C + m_Name偏移] → "Enemy"
        //      输出: [2] 节点3地址  BaseObject_C地址  Enemy
        //      读取 [节点3 + 0x08] → gomHead ← next 指回了头节点！
        //      currentNode = gomHead
        //
        //    循环 4：
        //      currentNode == gomHead  → 遍历结束！
        //      （因为是循环链表，走一圈就会回到头节点）
        //
        // ================================================================

        // 从第一个节点开始遍历
        uintptr_t currentNode = firstNode;

        // 统计变量
        size_t count = 0;        // 已处理的节点总数
        size_t validCount = 0;   // 有效对象（BaseObject 不为空）的数量
        size_t invalidCount = 0; // 无效节点（BaseObject 为空）的数量

        // 打印表头
        // 格式：序号 | 节点地址 | 对象地址 | 名称
        printf("%-6s  %-18s  %-18s  %s\n", "Index", "NodeAddr", "ObjectAddr", "Name");
        printf("------  ------------------  ------------------  ------------------------------\n");

        // ---- 主循环：沿着 next 指针遍历 ----
        while (true)
        {
            // ---- 检查 1：是否回到了 GOM 头节点（循环链表的终点）----
            // 因为是循环链表，遍历完所有节点后，next 会指回头节点
            if (currentNode == gomHead)
            {
                printf("[*] 已遍历回 GOM 头节点，循环链表遍历完成\n");
                break;
            }

            // ---- 检查 2：当前节点地址是否为空（链表损坏的保护）----
            if (!currentNode)
            {
                printf("[-] TraverseGOM: 遇到空节点指针，终止遍历\n");
                break;
            }

            // ---- 读取当前节点中的 BaseObject 指针 ----
            // [currentNode + 0x10] 存的是一个指针，指向真正的游戏对象
            uintptr_t baseObject = *reinterpret_cast<uintptr_t*>(currentNode + 0x10);

            if (baseObject && detectedNameOffset >= 0)
            {
                // ---- 情况 A：有有效的 BaseObject 且知道 m_Name 偏移 ----
                // 可以读出对象名称！

                // 读取 [BaseObject + m_Name偏移]，得到一个指针，指向名称字符串
                uintptr_t namePtr = *reinterpret_cast<uintptr_t*>(baseObject + detectedNameOffset);

                char nameBuf[128] = {};
                const char* name = "<null>";  // 默认显示 <null>

                if (namePtr && SafeReadString(namePtr, nameBuf, sizeof(nameBuf)))
                    name = nameBuf;  // 成功读到字符串

                // 输出一行：序号、节点地址、对象地址、名称
                printf("[%4zu]  0x%p  0x%p  %s\n",
                       count, (void*)currentNode, (void*)baseObject, name);
                validCount++;
            }
            else if (baseObject)
            {
                // ---- 情况 B：有 BaseObject 但不知道 m_Name 偏移 ----
                // 只能输出地址，名称显示 <unknown offset>
                printf("[%4zu]  0x%p  0x%p  <unknown offset>\n",
                       count, (void*)currentNode, (void*)baseObject);
                validCount++;
            }
            else
            {
                // ---- 情况 C：BaseObject 指针为空 ----
                // 这个节点没有关联的游戏对象（可能是已销毁但未从链表移除的节点）
                printf("[%4zu]  0x%p  <null>            <no object>\n",
                       count, (void*)currentNode);
                invalidCount++;
            }

            count++;

            // ---- 安全限制 1：用户指定的最大遍历数量 ----
            if (maxCount > 0 && count >= maxCount)
            {
                printf("[*] 已达到最大遍历数量 %zu，停止遍历\n", maxCount);
                break;
            }

            // ---- 安全限制 2：防止无限循环的硬上限 ----
            // 正常游戏不会超过 10 万个对象，如果到了说明链表可能有问题
            if (count >= 100000)
            {
                printf("[-] TraverseGOM: 已达到安全上限 100000，停止遍历\n");
                break;
            }

            // ---- 沿着 next 指针跳到下一个节点 ----
            // 这是链表遍历的核心操作：[currentNode + 0x08] 存着 next 指针
            uintptr_t nextNode = *reinterpret_cast<uintptr_t*>(currentNode + 0x08);

            // 防止 next 指向自身（死循环保护）
            if (nextNode == currentNode)
            {
                printf("[-] TraverseGOM: next 指针指向自身，链表可能损坏\n");
                break;
            }

            // 移动到下一个节点
            currentNode = nextNode;
        }


        // ================================================================
        //  Step 6: 输出遍历统计信息
        // ================================================================
        printf("\n============ GOM 遍历结果 ============\n");
        printf("[*] 总节点数: %zu\n", count);
        printf("[*] 有效对象: %zu\n", validCount);
        printf("[*] 无效节点: %zu\n", invalidCount);
        if (detectedNameOffset >= 0)
            printf("[*] 名称偏移: +0x%X (自动检测)\n", detectedNameOffset);
        else
            printf("[-] 未检测到有效名称偏移，请根据 Hex Dump 手动确认\n");
        printf("============ GOM 遍历结束 ============\n\n");
    }

    // ================================================================
    // ClassDebugger —— 运行时类实例调试器实现
    // ================================================================

    namespace ClassDebugger
    {
        // 全局状态实例
        DebugState g_DebugState;

        // --------------------------------------------------------
        // SearchInstances() —— 搜索指定类名的所有实例
        // --------------------------------------------------------
        //
        // 【算法】
        //   1. 遍历 Domain 下所有 Assembly → Image
        //   2. 遍历 Image 中所有 Class，按类名模糊匹配
        //   3. 对匹配到的类，通过 GetTypeObject 获取 System.Type
        //   4. 调用 FindObjectsOfType(typeObj) 获取实例数组
        //   5. 缓存实例信息（地址、名称、世界坐标）
        //
        // 大小写不敏感的子串搜索（纯 CRT，无需额外库）
        static const char* StrStrI(const char* haystack, const char* needle)
        {
            if (!needle || !*needle) return haystack;
            if (!haystack) return nullptr;
            for (; *haystack; ++haystack)
            {
                const char* h = haystack;
                const char* n = needle;
                while (*h && *n && tolower((unsigned char)*h) == tolower((unsigned char)*n))
                {
                    ++h; ++n;
                }
                if (!*n) return haystack;
            }
            return nullptr;
        }

        // SEH 安全包装：获取实例名称（纯 C 接口避免 C2712）
        static void SafeGetInstanceName(void* pObj, const char* className,
            char* outName, size_t nameBufSize)
        {
            __try
            {
                // 尝试用 il2cpp_object_get_class 获取类名
                if (il2cpp_object_get_class && il2cpp_class_get_name)
                {
                    Il2CppClass* pClass = il2cpp_object_get_class(reinterpret_cast<Il2CppObject*>(pObj));
                    if (pClass)
                    {
                        const char* cn = il2cpp_class_get_name(pClass);
                        if (cn)
                        {
                            strncpy_s(outName, nameBufSize, cn, _TRUNCATE);
                            return;
                        }
                    }
                }
                strncpy_s(outName, nameBufSize, className, _TRUNCATE);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                strncpy_s(outName, nameBufSize, className, _TRUNCATE);
            }
        }

        // SEH 安全：直接对 GameObject 获取 Transform→Position
        static bool TryGetPosAsGameObject(void* pObj, Unity_Vector3& outPos)
        {
            __try
            {
                void* pTransform = Engine::GetTransform(pObj);
                if (pTransform)
                {
                    outPos = Engine::GetPosition(pTransform);
                    return true;
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
            return false;
        }

        // SEH 安全：对 Component/MonoBehaviour 通过 get_gameObject 获取位置
        static bool TryGetPosViaComponent(void* pObj, Unity_Vector3& outPos)
        {
            __try
            {
                if (!il2cpp_object_get_class || !il2cpp_class_get_name ||
                    !il2cpp_class_get_parent || !il2cpp_class_get_method_from_name ||
                    !il2cpp_runtime_invoke)
                    return false;

                Il2CppClass* pClass = il2cpp_object_get_class(reinterpret_cast<Il2CppObject*>(pObj));
                // 遍历继承链，找到 Component 基类
                Il2CppClass* compClass = nullptr;
                while (pClass)
                {
                    const char* cn = il2cpp_class_get_name(pClass);
                    if (cn && (strcmp(cn, "Component") == 0 ||
                               strcmp(cn, "Behaviour") == 0 ||
                               strcmp(cn, "MonoBehaviour") == 0))
                    {
                        compClass = pClass;
                        break;
                    }
                    pClass = il2cpp_class_get_parent(pClass);
                }

                if (!compClass) return false;

                // 反射调用 Component.get_gameObject
                const MethodInfo* goMethod = il2cpp_class_get_method_from_name(compClass, "get_gameObject", 0);
                if (!goMethod) return false;

                Il2CppException* exc = nullptr;
                Il2CppObject* pGO = il2cpp_runtime_invoke(goMethod, pObj, nullptr, &exc);
                if (!pGO || exc) return false;

                // 对得到的 GameObject 获取 Transform→Position
                void* pTransform = Engine::GetTransform(pGO);
                if (pTransform)
                {
                    outPos = Engine::GetPosition(pTransform);
                    return true;
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
            return false;
        }

        // 获取世界坐标：先尝试 GameObject 路径，再尝试 Component 路径
        bool SafeGetInstancePosition(void* pObj, Unity_Vector3& outPos)
        {
            if (TryGetPosAsGameObject(pObj, outPos))
                return true;
            if (TryGetPosViaComponent(pObj, outPos))
                return true;
            return false;
        }

        // 内部实现（不带 ThreadGuard，由 Update 调用，Update 已 attach）
        static void SearchInstancesInternal()
        {
            // 不再 clear，改为合并模式：新的加入，消失的移除，不变的保留
            g_DebugState.searchDone = false;

            const char* targetName = g_DebugState.classNameBuf;
            if (!targetName || targetName[0] == '\0')
            {
                g_DebugState.statusMsg = "[错误] 请输入类名";
                return;
            }

            // 前置检查
            if (!il2cpp_domain_get || !il2cpp_domain_get_assemblies ||
                !il2cpp_assembly_get_image || !il2cpp_image_get_class_count ||
                !il2cpp_image_get_class || !il2cpp_class_get_name ||
                !FindObjectsOfType)
            {
                g_DebugState.statusMsg = "[错误] IL2CPP API 未初始化";
                return;
            }

            Il2CppDomain* pDomain = il2cpp_domain_get();
            if (!pDomain)
            {
                g_DebugState.statusMsg = "[错误] 无法获取 Domain";
                return;
            }

            size_t assemblyCount = 0;
            const Il2CppAssembly** assemblies = il2cpp_domain_get_assemblies(pDomain, &assemblyCount);
            if (!assemblies || assemblyCount == 0)
            {
                g_DebugState.statusMsg = "[错误] 没有找到任何程序集";
                return;
            }

            // 收集本轮搜索到的新实例列表
            std::vector<InstanceInfo> newInstances;
            int matchedClasses = 0;

            // 遍历所有程序集
            for (size_t a = 0; a < assemblyCount; a++)
            {
                const Il2CppAssembly* pAssembly = assemblies[a];
                if (!pAssembly) continue;

                const Il2CppImage* pImage = il2cpp_assembly_get_image(pAssembly);
                if (!pImage) continue;

                size_t classCount = il2cpp_image_get_class_count(pImage);

                // 遍历 Image 中所有类
                for (size_t c = 0; c < classCount; c++)
                {
                    const Il2CppClass* pClass = il2cpp_image_get_class(pImage, c);
                    if (!pClass) continue;

                    const char* className = il2cpp_class_get_name(const_cast<Il2CppClass*>(pClass));
                    if (!className) continue;

                    // 模糊匹配：类名包含目标字符串（大小写不敏感）
                    if (!StrStrI(className, targetName))
                        continue;

                    matchedClasses++;

                    // 获取 System.Type 实例
                    const Il2CppType* pType = il2cpp_class_get_type(const_cast<Il2CppClass*>(pClass));
                    if (!pType) continue;

                    Il2CppObject* pTypeObj = il2cpp_type_get_object(pType);
                    if (!pTypeObj) continue;

                    // 调用 FindObjectsOfType
                    System_Object_array* ObjArray = reinterpret_cast<System_Object_array*>(
                        FindObjectsOfType(pTypeObj));
                    if (!ObjArray || ObjArray->max_length <= 0) continue;

                    size_t count = static_cast<size_t>(ObjArray->max_length);

                    // 缓存每个实例到 newInstances
                    for (size_t i = 0; i < count; i++)
                    {
                        void* pObj = ObjArray->m_Items[i];
                        if (!pObj) continue;

                        InstanceInfo info;
                        info.pObject = pObj;

                        // 获取名称（异常时用类名代替）
                        char nameBuf[256] = "";
                        SafeGetInstanceName(pObj, className, nameBuf, 256);
                        info.name = nameBuf;

                        // 获取世界坐标（异常时坐标保持默认 0,0,0）
                        bool hasPos = SafeGetInstancePosition(pObj, info.worldPos);
                        info.hasValidPos = hasPos;

                        newInstances.push_back(info);
                    }
                }
            }

            // ---- 合并逻辑：新增的加入，消失的移除，不变的保留 ----

            // 构建新实例指针集合
            std::unordered_set<void*> newPtrSet;
            for (auto& inst : newInstances)
                newPtrSet.insert(inst.pObject);

            // 构建旧实例指针集合
            std::unordered_set<void*> oldPtrSet;
            for (auto& inst : g_DebugState.instances)
                oldPtrSet.insert(inst.pObject);

            // 移除旧列表中已不存在的实例（被销毁/GC回收）
            size_t removedCount = 0;
            g_DebugState.instances.erase(
                std::remove_if(g_DebugState.instances.begin(), g_DebugState.instances.end(),
                    [&newPtrSet, &removedCount](const InstanceInfo& inst) -> bool {
                        if (newPtrSet.find(inst.pObject) == newPtrSet.end())
                        {
                            removedCount++;
                            return true;
                        }
                        return false;
                    }),
                g_DebugState.instances.end());

            // 添加新列表中新增的实例
            size_t addedCount = 0;
            for (auto& inst : newInstances)
            {
                if (oldPtrSet.find(inst.pObject) == oldPtrSet.end())
                {
                    g_DebugState.instances.push_back(inst);
                    addedCount++;

                    // 控制台只输出新增实例
                    printf("[+] ClassDebugger: 新实例 %s at 0x%p  pos(%.1f, %.1f, %.1f)%s\n",
                           inst.name.c_str(), inst.pObject,
                           inst.worldPos.x, inst.worldPos.y, inst.worldPos.z,
                           inst.hasValidPos ? "" : " [坐标获取失败]");
                }
            }

            // 输出被移除的实例
            for (void* oldPtr : oldPtrSet)
            {
                if (newPtrSet.find(oldPtr) == newPtrSet.end())
                {
                    printf("[-] ClassDebugger: 实例已消失 0x%p\n", oldPtr);
                }
            }

            // 更新状态消息
            char buf[256];
            if (matchedClasses == 0)
            {
                sprintf_s(buf, "[未找到] 没有匹配 \"%s\" 的类", targetName);
            }
            else
            {
                size_t keptCount = g_DebugState.instances.size() - addedCount;
                sprintf_s(buf, "[运行中] %zu 个实例 (新增%zu, 移除%zu, 保留%zu)",
                         g_DebugState.instances.size(), addedCount, removedCount, keptCount);
            }
            g_DebugState.statusMsg = buf;
            g_DebugState.searchDone = true;
            g_DebugState.lastSearchTime = static_cast<float>(ImGui::GetTime());

            // 首次搜索时输出完整列表
            if (oldPtrSet.empty() && !g_DebugState.instances.empty())
            {
                printf("[*] ClassDebugger: 首次搜索完成，共 %zu 个实例\n", g_DebugState.instances.size());
                int idx = 0;
                for (auto& inst : g_DebugState.instances)
                {
                    printf("  [%d] %s at 0x%p  pos(%.1f, %.1f, %.1f)%s\n",
                           idx++, inst.name.c_str(), inst.pObject,
                           inst.worldPos.x, inst.worldPos.y, inst.worldPos.z,
                           inst.hasValidPos ? "" : " [坐标获取失败]");
                }
            }
            else if (addedCount > 0 || removedCount > 0)
            {
                printf("[*] ClassDebugger: %s\n", g_DebugState.statusMsg.c_str());
            }
        }

        // 公开接口（带 ThreadGuard，供按钮手动调用）
        void SearchInstances()
        {
            ThreadGuard guard;
            if (!guard.IsValid())
            {
                g_DebugState.statusMsg = "[错误] 无法 attach IL2CPP 线程";
                return;
            }
            SearchInstancesInternal();
        }

        // --------------------------------------------------------
        // CheckAutoSearch() —— 检查自动搜索定时（每帧调用）
        // --------------------------------------------------------
        //
        // 【职责】
        //   1. 总开关 enabled 为 false 时，跳过
        //   2. 检查是否到达自动搜索间隔，到达时调用 SearchInstancesInternal()
        //   3. 保持 ThreadGuard 覆盖整个搜索过程
        //
        void CheckAutoSearch()
        {
            if (!g_DebugState.enabled)
                return;

            float now = static_cast<float>(ImGui::GetTime());
            if (g_DebugState.lastSearchTime < 0 ||
                (now - g_DebugState.lastSearchTime) >= g_DebugState.autoSearchInterval)
            {
                ThreadGuard guard;
                if (!guard.IsValid())
                    return;

                SearchInstancesInternal();
            }
        }

    } // namespace ClassDebugger

} // namespace Engine
