// ============================================================================
// 文件：src/Hook/HookManager.h
// 模块：Hook 层 - Hook 管理器接口
// ============================================================================
// 【用途】
//   统一管理所有 Hook 的初始化和卸载。
//   当前管理的 Hook：
//   - PresentHook（DX11 虚表 Hook，拦截 Present/ResizeBuffers）
//
// 【扩展指南】
//   添加新的 Hook（如 DrawIndexed Hook、CreateTexture Hook 等）：
//   1. 在 src/Hook/ 目录下创建新的 Hook 文件（如 DrawHook.h/cpp）
//   2. 在 InitializeAllHooks() 中调用新 Hook 的初始化函数
//   3. 在 ShutdownAllHooks() 中调用新 Hook 的卸载函数
// ============================================================================

#pragma once

namespace Hook
{
    /// 初始化所有 Hook（包括 Present Hook 和 ImGui）。
    /// 在 Engine::Initialize() 之后调用。
    /// @return 所有 Hook 均成功返回 true，任意失败返回 false
    bool InitializeAllHooks();

    /// 卸载所有 Hook（游戏退出或 DLL 卸载时调用）。
    void ShutdownAllHooks();

} // namespace Hook
