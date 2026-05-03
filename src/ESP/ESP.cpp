// ============================================================================
// 文件：src/ESP/ESP.cpp
// 模块：ESP 层 - 所有 ESP 绘制功能的实现
// ============================================================================
//
// 【架构定位】
//   ESP 层负责「把游戏数据画到屏幕上」。
//   数据来源：调用 Engine 层的函数指针和工具函数
//   绘制方式：ImGui DrawList（前景层 / 窗口层）
//
// 【线程安全】
//   本文件中的函数在 DX11 Present Hook 的渲染线程上调用，
//   调用 IL2CPP API 前必须 il2cpp_thread_attach，返回前必须 detach。
//
// ============================================================================

#include "ESP.h"
#include "Engine/Engine.h"     // 包含此头文件即可获得所有 IL2CPP V类型/函数指针/游戏方法声明
#include "Engine/class.h"      // Camera, Vector3, Unity_Vector3 等游戏类型
#include "UI/Menu/MenuSwitches.h"
#include "UI/Draw/Draw.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include <cstdio>
#include <algorithm>           // std::remove_if
#include <string>

namespace ESP
{

    // ========================================================================
    // Render() —— ESP 总入口
    // ========================================================================
    // 在 hkPresent 中每帧调用，内部根据开关分发到各子函数。
    // 新增 ESP 功能时在此处添加调用即可。
    void Render()
    {
        DrawAllObjectPointers();
        Engine::ClassDebugger::CheckAutoSearch();
        DrawClassDebuggerInstances();
        DrawEnemyESP();
        // 后续在此添加：
        // DrawESPBox();
        // DrawESPName();
        // DrawESPSkeleton();
        // ...
    }

    // ========================================================================
    // DrawAllObjectPointers() —— 绘制所有 GameObject 的指针地址到屏幕
    // ========================================================================
    //
    // 【实现思路】
    //   1. 获取主摄像机（Camera.get_main()）
    //   2. 通过 FindObjectsOfType 获取所有 GameObject 数组
    //   3. 遍历每个 GameObject：
    //      a. 获取 Transform → 获取世界坐标
    //      b. 通过 Camera.WorldToScreen 将 3D 坐标投影到 2D 屏幕坐标
    //      c. 在屏幕对应位置用 ImGui 绘制指针地址文字
    //
    void DrawAllObjectPointers()
    {
        // 检查开关是否启用
        if (!MenuSwitches::g_Toggles.DrawObjectPointers)
            return;

        // ---- 线程安全：RAII 自动管理 IL2CPP 线程 attach/detach ----
        // 构造时自动 attach，离开作用域（含中途 return）自动 detach
        Engine::ThreadGuard guard;
        if (!guard.IsValid()) return;

        // 获取主摄像机
        Camera* pCamera = Engine::GetMainCameraSafe();
        if (!pCamera) return;

        // ---- 一行获取 GameObject 的 System.Type 实例 ----
        // 之前需要 6 步链式调用（domain→assembly→image→class→type→typeObj）
        // + 每步 null 检查 + 每步 detach，现在一行搞定
        Il2CppObject* pTypeObj = Engine::GetTypeObject("UnityEngine.CoreModule", "UnityEngine", "GameObject");
        if (!pTypeObj) return;

        // 调用 FindObjectsOfType 获取所有 GameObject
        if (!FindObjectsOfType) return;
        System_Object_array* ObjArray = reinterpret_cast<System_Object_array*>(FindObjectsOfType(pTypeObj));
        if (!ObjArray || ObjArray->max_length <= 0) return;

        size_t total = static_cast<size_t>(ObjArray->max_length);

        // 获取 ImGui 前景绘制列表
        ImDrawList* drawList = ImGui::GetForegroundDrawList();

        for (size_t i = 0; i < total; i++)
        {
            void* pGameObject = ObjArray->m_Items[i];
            if (!pGameObject) continue;

            // 获取 Transform → 获取世界坐标
            void* pTransform = Engine::GetTransform(pGameObject);
            if (!pTransform) continue;

            Unity_Vector3 worldPos = Engine::GetPosition(pTransform);

            // 将 Unity_Vector3 转换为 Vector3（Types.h 中的类型，Camera::WorldToScreen 使用）
            Vector3 pos3D(worldPos.x, worldPos.y, worldPos.z);
            Vector2 screenPos;

            // WorldToScreen 投影
            if (pCamera->WorldToScreen(pos3D, screenPos))
            {
                // 格式化指针地址为字符串
                char buf[32];
                sprintf_s(buf, "%p", pGameObject);

                // 在屏幕上绘制指针地址（黄色文字）
                drawList->AddText(
                    ImVec2(screenPos.X, screenPos.Y),
                    IM_COL32(255, 255, 0, 255),
                    buf
                );
            }
        }
    }

    // ========================================================================
    // DrawClassDebuggerInstances() —— 绘制 ClassDebugger 搜索到的实例
    // ========================================================================
    //
    // 【实现思路】
    //   1. 每帧刷新所有实例位置，移除已被 GC 回收的
    //   2. 遍历有效实例，距离过滤后投影到屏幕
    //   3. 用 ImGui 绘制名称+地址标签
    //

    // SEH 安全：刷新单个实例的位置，返回 true 表示对象仍然存活
    static bool RefreshInstancePosition(Engine::ClassDebugger::InstanceInfo& inst)
    {
        __try
        {
            Unity_Vector3 newPos = {0, 0, 0};
            bool ok = Engine::ClassDebugger::SafeGetInstancePosition(inst.pObject, newPos);
            if (ok)
            {
                inst.worldPos = newPos;
                inst.hasValidPos = true;
            }
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    // SEH 安全包装：WorldToScreen（独立函数避免 C2712）
    static bool SafeWorldToScreen(Camera* pCamera, const Vector3& pos3D, Vector2& screenPos)
    {
        __try
        {
            return pCamera->WorldToScreen(pos3D, screenPos);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    void DrawClassDebuggerInstances()
    {
        auto& state = Engine::ClassDebugger::g_DebugState;
        if (!state.enabled || state.instances.empty())
            return;

        // 保持线程 attach，避免 RefreshInstancePosition 中调用 IL2CPP API 时 GC 崩溃
        Engine::ThreadGuard guard;
        if (!guard.IsValid()) return;

        // 获取主摄像机
        Camera* pCamera = Engine::GetMainCameraSafe();
        if (!pCamera) return;

        ImDrawList* drawList = ImGui::GetForegroundDrawList();
        if (!drawList) return;

        // 每帧刷新实例位置，移除已被 GC 回收的实例
        // 不使用 std::remove_if + lambda，因为 SEH 异常跳转可能破坏迭代器，
        // 导致 ImGui 内部 ImVector 状态不一致触发断言
        {
            size_t writeIdx = 0;
            for (size_t readIdx = 0; readIdx < state.instances.size(); readIdx++)
            {
                if (RefreshInstancePosition(state.instances[readIdx]))
                {
                    if (writeIdx != readIdx)
                        state.instances[writeIdx] = std::move(state.instances[readIdx]);
                    writeIdx++;
                }
            }
            state.instances.resize(writeIdx);
        }

        if (state.instances.empty())
            return;

        // 文字颜色
        ImU32 col = ImGui::GetColorU32(
            ImVec4(state.textColor[0],
                   state.textColor[1],
                   state.textColor[2],
                   state.textColor[3]));

        for (auto& inst : state.instances)
        {
            if (!inst.hasValidPos)
                continue;

            // 距离过滤
            float dist = sqrtf(inst.worldPos.x * inst.worldPos.x +
                               inst.worldPos.y * inst.worldPos.y +
                               inst.worldPos.z * inst.worldPos.z);
            if (dist > state.maxDistance)
                continue;

            // 投影到屏幕
            Vector3 pos3D(inst.worldPos.x, inst.worldPos.y, inst.worldPos.z);
            Vector2 screenPos;

            if (!SafeWorldToScreen(pCamera, pos3D, screenPos))
                continue;

            // 构造显示文本
            std::string label;
            if (state.showName)
                label += inst.name;
            if (state.showName && state.showAddress)
                label += " ";
            if (state.showAddress)
            {
                char addrBuf[32];
                sprintf_s(addrBuf, "0x%p", inst.pObject);
                label += addrBuf;
            }

            if (!label.empty())
            {
                drawList->AddText(
                    ImVec2(screenPos.X, screenPos.Y),
                    col, label.c_str());
            }
        }
    }

    // ========================================================================
    // DrawEnemyESP() —— 绘制 PVE 敌人位置到屏幕
    // ========================================================================
    //
    // 【功能概述】
    //   在游戏画面上方叠加绘制 PVE 模式下所有敌人的屏幕位置标记，
    //   供玩家实时掌握敌人方位信息。通过 ImGui 前景绘制层实现，
    //   不修改游戏内存或渲染管线，纯只读 + 叠加绘制。
    //
    // 【数据来源链路】
    //   游戏内部 C# 对象关系：
    //     GameManager_PVE (单例管理器)
    //       └─ PVE_EnimyList  (List<GameObject>, 偏移 0x298)
    //            └─ [0..N]     (每个元素是 GameObject*，指向场景中的敌人对象)
    //
    //   IL2CPP 内存布局（class.h 中定义）：
    //     GameManager_PVE
    //       0x000~0x22F: 头部 + 各种字段（padding）
    //       0x298:       Unity_List* PVE_EnimyList
    //
    //     Unity_List (对应 C# List<T>)
    //       0x00: Il2CppObject 头部 (16字节)
    //       0x10: Unity_Array* Objects  → 底层 T[] 数组
    //       0x18: int Count             → 实际元素个数（_size，直接值非指针）
    //       0x1C: int Version           → 版本号
    //
    //     Unity_Array (对应 C# T[])
    //       0x00: Il2CppObject 头部 (16字节)
    //       0x10: int max_length        → 数组容量
    //       0x18: void* Objects[N]      → 连续存储的元素指针数组
    //
    //   因此遍历路径为：
    //     pGame->PVE_EnimyList->Objects->Objects[i]
    //     ~~~~~~  ~~~~~~~~~~~~~  ~~~~~~~  ~~~~~~~~~~
    //     管理器   List<GameObject>  T[]数组   第i个GameObject*
    //     两级指针解引用：List._items → Array.m_Items[i]
    //
    // 【坐标转换流程】
    //   1. 世界坐标 (World Space)
    //      ├── 通过 GameObject.get_transform() 获取 Transform 组件
    //      └── 通过 Transform.get_position() 获取世界坐标 (x, y, z)
    //
    //   2. 屏幕坐标 (Screen Space)
    //      ├── 通过 Camera.WorldToScreen() 将 3D 坐标投影到 2D
    //      ├── X: 屏幕水平像素位置（0 = 左边缘）
    //      ├── Y: 屏幕垂直像素位置（0 = 上边缘，Y 轴向下）
    //      └── 投影失败 = 对象在摄像机背后或视野外
    //
    // 【线程安全】
    //   本函数在 DX11 Present Hook 的渲染线程上执行，而 IL2CPP GC 不感知此线程。
    //   必须通过 ThreadGuard 进行 il2cpp_thread_attach/detach，否则：
    //     - 访问托管对象时 GC 可能正在移动它 → 读到垃圾数据或崩溃
    //     - IL2CPP 内部线程检查失败 → 访问违例
    //   ThreadGuard 采用 RAII 模式，构造时 attach，析构时自动 detach。
    //
    // 【SEH 保护】
    //   遍历敌人列表时，对象可能被 Unity GC 在任意时刻回收：
    //     - pEnemyObj 指针仍指向旧地址，但底层内存已释放
    //     - 调用 GetTransform/GetPosition 时触发访问违例
    //   SEH (__try/__except) 捕获此类异常，跳过已回收的对象。
    //   注意：MSVC 不允许在含有 C++ 析构对象（如 ThreadGuard）的函数中使用 __try，
    //   因此 TryGetEnemyWorldPos 必须是独立的静态函数。
    //
    // 【绘制方式】
    //   使用 ImGui::GetForegroundDrawList() 获取最顶层绘制列表，
    //   绘制内容会叠加在游戏画面 + ImGui 窗口之上，不会被遮挡。
    //   绘制操作不会修改游戏渲染状态，仅在 Present 之前插入额外绘制指令。
    //
    // 【调用时机】
    //   每帧由 ESP::Render() 调用，而 Render() 由 hkPresent() 调用。
    //   调用链：DX11 Present → hkPresent → ESP::Render → DrawEnemyESP
    //
    // 【UI 控制】
    //   开关：MenuSwitches::g_Toggles.ShowEnemyESP（菜单中的"绘制敌人位置"复选框）
    //

    // TryGetEnemyWorldPos —— SEH 安全地获取敌人世界坐标
    // 独立静态函数，因为 MSVC 不允许含析构对象的函数使用 __try（C2712 错误）
    // 参数：pEnemyObj — 敌人 GameObject 指针; outPos — 输出世界坐标
    // 返回：true 成功获取; false 对象无效或已被 GC 回收
    static bool TryGetEnemyWorldPos(void* pEnemyObj, Vector3& outPos)
    {
        __try
        {
            // 第一步：GameObject → Transform
            // 调用 C# GameObject.get_transform()，返回 Transform* 指针
            void* pTransform = Engine::GetTransform(pEnemyObj);
            if (!pTransform) return false;

            // 第二步：Transform → 世界坐标
            // 调用 C# Transform.get_position()，返回 Unity_Vector3 (x, y, z)
            Unity_Vector3 pos = Engine::GetPosition(pTransform);

            // 类型转换：Unity_Vector3 → Vector3（Camera::WorldToScreen 需要 Vector3）
            outPos = Vector3(pos.x, pos.y, pos.z);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            // 对象已被 GC 回收，内存不可访问 → 捕获异常，返回失败
            return false;
        }
    }

    void DrawEnemyESP()
    {
        // ---- 开关检查 ----
        // 用户未启用则直接返回，避免无谓的 IL2CPP 调用开销
        if (!MenuSwitches::g_Toggles.ShowEnemyESP)
            return;

        // ---- 线程安全 ----
        // RAII：构造时 il2cpp_thread_attach，析构时 il2cpp_thread_detach
        // 必须在访问任何 IL2CPP 托管对象之前执行
        Engine::ThreadGuard guard;
        if (!guard.IsValid()) return;

        // ---- 获取主摄像机 ----
        // 调用 Camera.get_main()，返回场景中标记为 MainCamera 的摄像机
        // WorldToScreen 投影需要此摄像机的 ViewProjection 矩阵
        Camera* pCamera = Engine::GetMainCameraSafe();
        if (!pCamera)
        {
            static bool logged_noCam = false;
            if (!logged_noCam) { printf("[ESP] DrawEnemyESP: 主摄像机为空\n"); logged_noCam = true; }
            return;
        }

        // ---- 获取 GameManager_PVE 的 System.Type 实例 ----
        // FindObjectsOfType 需要 System.Type 参数，通过 IL2CPP API 链式查找：
        //   domain_get → domain_assembly_open("Assembly-CSharp")
        //   → assembly_get_image → class_from_name(namespace, classname)
        //   → class_get_type → type_get_object → Il2CppObject* (System.Type)
        //
        // 命名空间为 "Assets.Scripts"（由 il2cpp.h 中 VTable 名称确认）
        // 回退尝试空命名空间（某些构建可能命名空间不同）
        Il2CppObject* pTypeObj = Engine::GetTypeObject("Assembly-CSharp", "Assets.Scripts", "GameManager_PVE");
        if (!pTypeObj)
        {
            // 回退：尝试空命名空间
            pTypeObj = Engine::GetTypeObject("Assembly-CSharp", "", "GameManager_PVE");
        }
        if (!pTypeObj)
        {
            static bool logged = false;
            if (!logged) { printf("[ESP] DrawEnemyESP: 无法获取 GameManager_PVE 的 System.Type\n"); logged = true; }
            return;
        }

        // ---- 查找 GameManager_PVE 运行时实例 ----
        // 调用 C# UnityEngine.Object.FindObjectsOfType(Type)
        // 返回 Unity_Array，包含所有活着的 GameManager_PVE 实例指针
        // 通常只有一个（PVE 模式下的游戏管理器单例）
        if (!FindObjectsOfType) return;
        Unity_Array* pGMArray = FindObjectsOfType(pTypeObj);
        if (!pGMArray || pGMArray->Count == 0) return;

        // ---- 取第一个 GameManager_PVE 实例 ----
        // pGMArray->Objects[0] 是 Il2CppObject*，强转为我们定义的内存布局结构
        // reinterpret_cast 安全前提：class.h 中的字段偏移与 IL2CPP 实际布局一致
        GameManager_PVE* pGame = reinterpret_cast<GameManager_PVE*>(pGMArray->Objects[0]);
        if (!pGame) return;

        // ---- 读取敌人列表 ----
        // PVE_EnimyList 偏移 0x298，类型是 Unity_List* (对应 C# List<GameObject>)
        // 两级指针：pEnemyList → Objects(Unity_Array*) → Objects[i](GameObject*)
        Unity_List* pEnemyList = pGame->PVE_EnimyList;
        if (!pEnemyList || !pEnemyList->Objects || pEnemyList->Count <= 0)
        {
            static bool logged_noList = false;
            if (!logged_noList) { printf("[ESP] PVE_EnimyList 为空或无数据 (ptr=%p, Obj=%p, Count=%d)\n", (void*)pEnemyList, pEnemyList ? (void*)pEnemyList->Objects : nullptr, pEnemyList ? pEnemyList->Count : 0); logged_noList = true; }
            return;
        }

        // ---- 获取 ImGui 绘制列表 ----
        // GetForegroundDrawList() 返回最顶层绘制层，覆盖在所有游戏内容之上
        ImDrawList* drawList = ImGui::GetForegroundDrawList();
        if (!drawList) return;

        // ---- 遍历敌人列表，逐一投影绘制 ----
        int enemyCount = pEnemyList->Count;
        int drawnCount = 0;     // 成功绘制数
        int transformFail = 0;  // GetTransform/GetPosition 失败数（对象被 GC 或非 GameObject）
        int w2sFail = 0;        // WorldToScreen 失败数（对象在视野外/相机背后）

        for (int i = 0; i < enemyCount && i < 100; ++i)
        {
            // 两级解引用：List._items(Unity_Array*) → Array.m_Items[i](GameObject*)
            void* pEnemyObj = pEnemyList->Objects->Objects[i];
            if (!pEnemyObj) continue;

            // ---- 获取敌人世界坐标（SEH 保护）----
            // 可能失败的原因：
            //   1. 对象已被 GC 回收，内存不可访问 → SEH 捕获
            //   2. 对象不是标准 GameObject，无 Transform 组件 → GetTransform 返回 null
            Vector3 worldPos;
            if (!TryGetEnemyWorldPos(pEnemyObj, worldPos))
            {
                transformFail++;
                static bool logged_tf = false;
                if (!logged_tf) { printf("[ESP] Enemy[%d] GetTransform/GetPosition 失败, obj=%p\n", i, pEnemyObj); logged_tf = true; }
                continue;
            }

            // ---- 3D → 2D 屏幕投影 ----
            // 使用摄像机的 ViewProjection 矩阵将世界坐标变换为屏幕像素坐标
            // 失败 = 对象在摄像机背后 (Z < 0) 或超出视锥体
            Vector2 screenPos;
            if (!pCamera->WorldToScreen(worldPos, screenPos))
            {
                w2sFail++;
                static bool logged_w2s = false;
                if (!logged_w2s) { printf("[ESP] Enemy[%d] WorldToScreen 失败, worldPos=(%.1f,%.1f,%.1f)\n", i, worldPos.X, worldPos.Y, worldPos.Z); logged_w2s = true; }
                continue;
            }

            // ---- 绘制红色方块标记（8x8 像素）----
            // 以敌人屏幕坐标为中心，绘制实心矩形
            // IM_COL32(R, G, B, A) — 红色，完全不透明
            float sz = 4.0f;
            drawList->AddRectFilled(
                ImVec2(screenPos.X - sz, screenPos.Y - sz),  // 左上角
                ImVec2(screenPos.X + sz, screenPos.Y + sz),  // 右下角
                IM_COL32(255, 50, 50, 255));                  // 暗红色

            // ---- 绘制序号标签 ----
            // 在方块右上方显示 "Enemy[0]"、"Enemy[1]" 等
            // 偏移 (8, -8) 避免与方块重叠
            char label[64];
            sprintf_s(label, "Enemy[%d]", i);
            drawList->AddText(
                ImVec2(screenPos.X + 8, screenPos.Y - 8),
                IM_COL32(255, 100, 100, 255),  // 浅红色文字
                label);
            drawnCount++;
        }

        // ---- 左上角状态面板 ----
        // 实时显示诊断数据，便于排查问题：
        //   total    — 列表中的敌人总数
        //   drawn    — 成功绘制到屏幕的数量
        //   tfFail   — Transform 获取失败数（对象无效/被GC）
        //   w2sFail  — 投影失败数（视野外/相机背后）
        char info[256];
        sprintf_s(info, "PVE ESP: total=%d drawn=%d tfFail=%d w2sFail=%d", enemyCount, drawnCount, transformFail, w2sFail);
        drawList->AddText(ImVec2(10, 30), IM_COL32(255, 80, 80, 255), info);
    }

} // namespace ESP
