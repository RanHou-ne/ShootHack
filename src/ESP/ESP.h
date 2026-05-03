// ============================================================================
// 文件：src/ESP/ESP.h
// 模块：ESP 层 - 所有 ESP 绘制功能的统一入口
// ============================================================================
//
// 【架构定位】
//   ESP 层专注于「屏幕绘制」—— 从 Engine 层获取游戏数据，
//   通过 ImGui DrawList 绘制到屏幕上。
//
//   调用链：
//     DX11Hook::hkPresent()（每帧）
//       → ESP::Render()（ESP 总入口）
//         → 各子函数（DrawAllObjectPointers / DrawESPBox / ...）
//
// 【扩展指南】
//   新增 ESP 绘制功能时：
//   1. 在 ESP 命名空间中声明新函数
//   2. 在 ESP.cpp 中实现（调用 Engine 层获取数据，调用 DrawUtils 绘制）
//   3. 在 Render() 中添加调用（受 MenuSwitches 开关控制）
//
// ============================================================================

#pragma once

namespace ESP
{
    // ---- ESP 总入口 ----
    // 在 hkPresent 中每帧调用，内部根据开关分发到各子函数
    void Render();

    // ---- 绘制所有 GameObject 的指针地址到屏幕 ----
    // 通过 FindObjectsOfType 获取所有 GameObject，
    // 将其世界坐标投影到屏幕坐标后显示指针地址
    void DrawAllObjectPointers();

    // ---- 绘制 ClassDebugger 搜索到的实例到屏幕 ----
    // 每帧刷新位置 + 投影绘制 + 移除已 GC 回收的实例
    void DrawClassDebuggerInstances();

    // ---- 绘制 PVE 敌人位置到屏幕 ----
    // 从 GameManager_PVE.PVE_EnimyList 遍历敌人，投影到屏幕绘制标记
    void DrawEnemyESP();

    

} // namespace ESP
