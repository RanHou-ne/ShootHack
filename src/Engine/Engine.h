// ============================================================================
// 文件：src/Engine/Engine.h
// 模块：引擎层 - IL2CPP 引擎接口声明
// ============================================================================
//
// 【整体架构总览 —— 这些文件之间的关系】
//
//   你的目标是：注入 DLL 到 Unity 游戏进程，然后调用游戏内部的 C# 方法。
//   但 C# 方法在 IL2CPP 编译后变成了 C++ 函数，你无法直接按名字调用。
//   所以你需要一个机制来「找到这些函数的地址」并「通过函数指针调用它们」。
//
//   整个引擎层分为三层：
//
//   ┌─────────────────────────────────────────────────────────────────────┐
//   │ 第1层：il2cpp_api.h + il2cpp_types.h                              │
//   │   IL2CPP 官方 C API 函数列表 + 核心结构体定义                        │
//   │   这些是「找到方法指针」的工具，所有 IL2CPP 游戏通用                    │
//   ├─────────────────────────────────────────────────────────────────────┤
//   │ 第2层：il2cpp_function.h                                          │
//   │   你想要调用的游戏 C# 方法列表（每个游戏不同）                         │
//   │   依靠第1层的 API 来定位方法地址                                     │
//   ├─────────────────────────────────────────────────────────────────────┤
//   │ 第3层：Engine.h + Engine.cpp                                      │
//   │   把第1层和第2层整合起来，提供完整的初始化和调用流程                     │
//   │   Initialize() 完成所有绑定，之后就能直接用函数指针调用游戏方法          │
//   └─────────────────────────────────────────────────────────────────────┘
//
// 【本文件的角色 —— 声明中心】
//
//   Engine.h 是所有函数指针的「声明中心」，它做了三件事：
//     1. 声明 IL2CPP 核心结构体（前向声明 + il2cpp_types.h 完整定义）
//     2. 用 X-Macro 展开 il2cpp_api.h → 生成所有 il2cpp_xxx 函数指针的声明
//     3. 用 X-Macro 展开 il2cpp_function.h → 生成所有游戏方法的函数指针声明
//
// 【X-Macro 技术详解 —— 为什么同一个 .h 文件要 #include 三次？】
//
//   传统写法：每个 API 手动写三遍（声明、定义、绑定），100 个 API 写 300 行。
//   X-Macro 写法：每个 API 只在列表中写一遍，用宏自动生成三份代码。
//
//   核心思想：il2cpp_api.h 只是一个「数据表」，不包含任何逻辑。
//   每次使用前 #define DO_API 为不同含义，再 #include，就会生成不同代码：
//
//   ┌────────────┬──────────────────────────────────────────────────────┐
//   │ 使用场景    │ #define DO_API(RetType, Name, Args) 展开为          │
//   ├────────────┼──────────────────────────────────────────────────────┤
//   │ Engine.h   │ using Name##_t = RetType(__stdcall*)Args;           │
//   │            │ extern Name##_t Name;  // 声明全局变量                │
//   ├────────────┼──────────────────────────────────────────────────────┤
//   │ Engine.cpp │ Name##_t Name = nullptr;  // 定义并初始化为空指针      │
//   │ (全局)     │                                              │
//   ├────────────┼──────────────────────────────────────────────────────┤
//   │ Engine.cpp │ Name = (Name##_t)GetProcAddress(hModule, #Name);    │
//   │ (Init内)   │  // 运行时通过 GetProcAddress 获取真实函数地址         │
//   └────────────┴──────────────────────────────────────────────────────┘
//
//   同理，il2cpp_function.h 的 DO_FUNC 也展开三次，只是多了三个参数：
//   AssemblyName、Namespace、ClassName，用于 GetMethod 定位。
//
// ============================================================================

#pragma once

#include "../Core/Base.h"
#include <vector>
#include <string>

// ============================================================================
// IL2CPP 核心结构体类型声明
// ============================================================================
//
// 【为什么有些是前向声明，有些是 #include？】
//
//   前向声明（typedef struct X X;）= 只告诉编译器"这个类型存在"
//     → 只能声明指针（X*），不能访问成员（x->field 会报错）
//     → 适用于我们只需要传递指针、不需要访问内部的类型
//
//   完整定义（#include "il2cpp_types.h"）= 告诉编译器结构体里有什么字段
//     → 可以访问成员（x->field）
//     → 适用于我们需要直接操作结构体内部的类型
//
//   判断标准：如果代码中有 xxx->member 这样的访问，就需要完整定义；
//            如果只作为函数参数传递指针，前向声明就够了。
//
typedef class Il2CppClass Il2CppClass;                      // 类元信息（只传指针，不访问成员 → 前向声明）
typedef struct Il2CppType Il2CppType;                      // 类型元信息（只传指针 → 前向声明）
typedef struct EventInfo EventInfo;                        // 事件成员
typedef struct MethodInfo MethodInfo;                      // 方法成员（只传指针 → 前向声明）
typedef struct FieldInfo FieldInfo;                        // 字段成员
typedef struct PropertyInfo PropertyInfo;                  // 属性成员
typedef struct Il2CppAssembly Il2CppAssembly;              // 程序集
typedef struct Il2CppArray Il2CppArray;                    // 数组对象
typedef struct Il2CppDelegate Il2CppDelegate;              // 委托对象
typedef struct Il2CppDomain Il2CppDomain;                  // 运行时域
typedef struct Il2CppImage Il2CppImage;                    // 模块镜像
typedef struct Il2CppException Il2CppException;            // 异常对象
typedef struct Il2CppProfiler Il2CppProfiler;              // 性能分析器

// Il2CppObject 和 System_Object_array 需要访问成员（->klass, ->max_length 等），
// 所以必须包含完整定义（从 sdk/il2cpp.h 提取的最小子集）
#include "il2cpp_types.h"

typedef struct Il2CppReflectionMethod Il2CppReflectionMethod; // 反射方法对象
typedef struct Il2CppReflectionType Il2CppReflectionType;     // 反射类型对象
typedef struct Il2CppString Il2CppString;                      // C# string 对象
typedef struct Il2CppThread Il2CppThread;                      // 线程对象
typedef struct Il2CppAsyncResult Il2CppAsyncResult;            // 异步结果对象
typedef struct Il2CppManagedMemorySnapshot Il2CppManagedMemorySnapshot;
typedef struct Il2CppCustomAttrInfo Il2CppCustomAttrInfo;

// ============================================================================
// X-Macro 第一阶段：声明 IL2CPP 官方 C API 函数指针
// ============================================================================
//
// 展开效果示例（以 il2cpp_domain_get 为例）：
//
//   DO_API(Il2CppDomain*, il2cpp_domain_get, ())
//   展开为：
//     using il2cpp_domain_get_t = Il2CppDomain*(__stdcall*)();  // 函数指针类型
//     extern il2cpp_domain_get_t il2cpp_domain_get;              // 全局变量声明
//
//   这样其他 .cpp 文件只要 #include "Engine.h" 就能使用这些函数指针
//
#define DO_API(RetType, Name, Args) \
    using Name##_t = RetType(__stdcall*)Args; \
    extern Name##_t Name;

#include "il2cpp_api.h"            // ← 展开：所有 IL2CPP C API 的声明

#undef DO_API                      // ← 解除宏定义，防止影响后面的代码


// ============================================================================
// X-Macro 第二阶段：声明游戏 C# 方法的函数指针
// ============================================================================
//
// DO_FUNC 比 DO_API 多三个参数（AssemblyName、Namespace、ClassName），
// 因为 C# 方法可能有重名（比如不同类都有 Update 方法），
// 需要靠完整路径来区分。
//
// 展开效果示例（以 FindObjectsOfType 为例）：
//
//   DO_FUNC(Unity_Array*, FindObjectsOfType, (void*), "UnityEngine.CoreModule", "UnityEngine", "Object")
//   展开为：
//     using FindObjectsOfType_t = Unity_Array*(__stdcall*)(void*);
//     extern FindObjectsOfType_t FindObjectsOfType;
//
#include "class.h"   // Unity_Array 的定义（il2cpp_function.h 中用到了这个类型）

#define DO_FUNC(RetType, Name, Args, AssemblyName, Namespace, ClassName, MethodName) \
    using Name##_t = RetType(__stdcall*)Args; \
    extern Name##_t Name;

#include "il2cpp_function.h"       // ← 展开：所有游戏 C# 方法的声明

#undef DO_FUNC

// ============================================================================
// 其他命名空间前向声明
// ============================================================================
namespace PresentHook
{
    IDXGISwapChain* GetSwapChain();   // 从 DX11 Present Hook 中获取 SwapChain
}

// ============================================================================
// Engine 命名空间 —— 引擎层的公共接口
// ============================================================================
namespace Engine
{
    // ---- 初始化（注入后必须调用一次）----
    // 完成窗口查找、DLL 加载、所有函数指针绑定
    void Initialize();

    // ---- 句柄获取 ----
    HWND GetHwnd();                   // Unity 主窗口句柄
    IDXGISwapChain* GetSwapChain();   // DX11 SwapChain（用于 ImGui 渲染）

    // ---- 工具函数 ----
    std::string GetTypeName(const Il2CppType* pType);  // 获取 Il2CppType 的可读名称
    std::string GetTypeName(std::string Name);          // 去掉命名空间前缀

    // ---- 核心：类/类型查找（封装 domain→assembly→image→class 链式调用）----
    //
    // 【为什么需要这两个函数？】
    //   每次想从 IL2CPP 运行时获取一个类或类型对象时，都要重复写：
    //     il2cpp_domain_get → il2cpp_domain_assembly_open → il2cpp_assembly_get_image
    //     → il2cpp_class_from_name → (可选) il2cpp_class_get_type → il2cpp_type_get_object
    //   这段代码在 ESP、TestMethod、DumpAssembly 等多处重复出现，封装后一行调用即可。
    //
    // GetClass：获取 Il2CppClass*（用于字段遍历、方法查找、DumpClass 等）
    //   参数：程序集名、命名空间、类名
    //   返回：Il2CppClass* 指针，失败返回 nullptr
    //
    // GetTypeObject：获取 Il2CppObject*（C# System.Type 实例，给 FindObjectsOfType 等需要 Type 参数的方法用）
    //   参数：程序集名、命名空间、类名
    //   返回：Il2CppObject* 指针，失败返回 nullptr
    Il2CppClass* GetClass(const char* AssemblyName,
                          const char* Namespace,
                          const char* ClassName);

    Il2CppObject* GetTypeObject(const char* AssemblyName,
                                const char* Namespace,
                                const char* ClassName);

    // ---- 核心：获取游戏方法指针 ----
    // 参数：程序集名、命名空间、类名、方法名
    // 返回：方法函数指针（通过 IL2CPP C API 链式查找）
    void* GetMethod(const char* AssemblyName,
                    const char* Namespace,
                    const char* ClassName,
                    const char* MethodName);

    // ---- IL2CPP 线程安全辅助 ----
    //
    // 【为什么需要这个？】
    //   DX11 Present Hook 运行在游戏渲染线程上，IL2CPP GC 不知道这个线程存在。
    //   调用任何 IL2CPP API 前必须 attach，返回前必须 detach，否则 GC 崩溃。
    //   每个函数都要写 attach/detach 样板代码，用 RAII 封装后自动管理。
    //
    // 用法：
    //   {
    //       Engine::ThreadGuard guard;        // 构造时自动 attach
    //       if (!guard.IsValid()) return;     // attach 失败
    //       // ... 调用 IL2CPP API ...
    //   }                                    // 离开作用域自动 detach
    class ThreadGuard
    {
    public:
        ThreadGuard();   // 自动 attach 当前线程到 IL2CPP
        ~ThreadGuard();  // 自动 detach
        bool IsValid() const { return m_pThread != nullptr; }
        Il2CppThread* GetThread() const { return m_pThread; }
        // 禁止拷贝和移动
        ThreadGuard(const ThreadGuard&) = delete;
        ThreadGuard& operator=(const ThreadGuard&) = delete;
    private:
        Il2CppThread* m_pThread = nullptr;
    };

    // ---- 动态遍历 ----

    // 打印对象的类型名 + 所有字段（偏移、名称、类型）
    void DumpObject(Il2CppObject* pObj);

    // 打印类的完整结构（字段、方法、属性）
    void DumpClass(Il2CppClass* pClass);

    // 打印指定程序集中所有类的名称
    void DumpAssembly(const char* AssemblyName);

    // ---- GameObject 信息读取 ----
    // 通过 C# 方法指针读取 GameObject 数据（绕过"薄包装器"问题）

    // 获取 GameObject 的名称（C# 托管字符串 → std::string）
    std::string GetGameObjectName(void* pGameObject);

    // 获取 GameObject 的 Transform 组件指针
    void* GetTransform(void* pGameObject);

    // 获取 Transform 的世界坐标
    Unity_Vector3 GetPosition(void* pTransform);

    // 获取 Transform 的世界旋转
    Unity_Quaternion GetRotation(void* pTransform);

    // 获取 Transform 的全局缩放
    Unity_Vector3 GetLossyScale(void* pTransform);

    // 打印 GameObject 的完整信息（名称、位置、旋转、缩放、层级、激活状态）
    void PrintGameObjectInfo(void* pGameObject);

    // ---- 摄像机相关 ----
    // 安全获取主摄像机（带 null 检查和日志）
    Camera* GetMainCameraSafe();

    // ---- 测试函数 ----
    void TestFindObjectsOfType();

    // 打印 需要的 地址到控制台
    void PrintNeedDataZz();


    




    // ========================================================================
    // ClassDebugger —— 运行时类实例调试器
    // ========================================================================
    // 用户输入类名，搜索该类所有实例，输出地址到控制台+屏幕
    namespace ClassDebugger
    {
        // 实例信息
        struct InstanceInfo
        {
            void*         pObject     = nullptr;   // 实例指针
            std::string   name;                     // 对象名称
            Unity_Vector3 worldPos    = {0,0,0};    // 世界坐标
            bool          hasValidPos = false;       // 坐标是否有效
        };

        // 调试器状态（由 UI 层读写）
        struct DebugState
        {
            char  classNameBuf[256] = "";       // 用户输入的类名
            bool  enabled          = false;     // 总开关：启用后自动循环搜索+绘制
            bool  showName         = true;      // 屏幕上显示对象名称
            bool  showAddress      = true;      // 屏幕上显示地址
            float maxDistance      = 500.0f;    // 最大显示距离
            float textColor[4]    = {0.0f, 1.0f, 0.5f, 1.0f}; // 文字颜色
            float autoSearchInterval = 3.0f;    // 自动搜索间隔（秒）

            // 运行时缓存（内部使用，UI 不直接操作）
            std::vector<InstanceInfo> instances;
            std::string  statusMsg;             // 状态信息（显示在 UI 日志区）
            bool         searchDone = false;    // 本次搜索是否完成
            float        lastSearchTime = -999.0f; // 上次搜索时间（ImGui 时间）
        };

        // 全局状态实例
        extern DebugState g_DebugState;

        // 搜索指定类名的所有实例（合并模式：新增加入，消失移除，不变保留）
        // 搜索结果合并到 g_DebugState.instances，同时输出变化摘要到 printf
        void SearchInstances();

        // 检查自动搜索定时，到达间隔时自动搜索（每帧调用）
        void CheckAutoSearch();

        // SEH 安全：获取实例世界坐标（先尝试 GameObject 路径，再尝试 Component 路径）
        bool SafeGetInstancePosition(void* pObj, Unity_Vector3& outPos);
    }

}
