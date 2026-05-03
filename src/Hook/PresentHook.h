// ============================================================================
// 文件：src/Hook/PresentHook.h
// 模块：Hook 层 - DX11 Present Hook 包装器
// ============================================================================
// 【用途】
//   对 DX11Hook 的轻量包装，提供统一的初始化/卸载接口，
//   并暴露 GetSwapChain() 供 Engine 层使用。
//
// 【调用链】
//   HookManager::InitializeAllHooks()
//     → PresentHook::Initialize()
//       → DX11HOOK()（虚表 Hook 安装）
//         → hkPresent()（每帧渲染入口）
//           → Menu::Render()
// ============================================================================

#pragma once

#include <d3d11.h>

namespace PresentHook
{
    /// 安装 DX11 虚表 Hook（Present + ResizeBuffers + WndProc）。
    /// @return 成功返回 true，失败返回 false
    bool Initialize();

    /// 卸载 Hook（当前为空实现，虚表 Hook 通过进程退出自动清理）。
    void Shutdown();

    /// 获取游戏的 SwapChain 指针（由 hkPresent 在首次调用时设置）。
    IDXGISwapChain* GetSwapChain();

} // namespace PresentHook
