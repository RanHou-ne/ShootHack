// ============================================================================
// 文件：src/Hook/DX11Hook.h
// 模块：Hook 层 - DX11 虚表 Hook 核心
// ============================================================================
// 【用途】
//   DX11 虚表（vtable）Hook 的核心实现。
//   通过创建临时 SwapChain 获取虚函数表地址，将 Present/ResizeBuffers
//   替换为自定义函数，从而在每个游戏帧中注入我们自己的渲染逻辑。
//
// 【Hook 原理（虚表 Hook / VTable Patching）】
//   1. 临时创建一个 D3D11 设备和 SwapChain
//   2. 从临时 SwapChain 对象的前 8 字节读取虚函数表地址
//   3. 虚表是一个函数指针数组，[8] = Present, [13] = ResizeBuffers
//   4. 修改内存保护属性为可写，替换 [8] 和 [13] 为我们的函数
//   5. 游戏后续调用 Present/Resize 时会跳转到我们的 hkPresent/hkResize
//
// 【关键函数】
//   DX11HOOK()     - 安装虚表 Hook（由 PresentHook::Initialize 调用）
//   hkPresent()    - 我们的 Present 替代函数（每帧渲染入口）
//   hkResize()     - 我们的 ResizeBuffers 替代函数（窗口大小改变时）
//   hkWndProc()    - 我们的窗口过程替代函数（处理键盘/鼠标输入）
//
// 【扩展指南】
//   如果要 Hook 更多 DX11 函数（如 DrawIndexed）：
//   1. 在 DX11HOOK() 中保存对应虚表索引的原始函数指针
//   2. 替换虚表条目为你的自定义函数
//   3. 在自定义函数中调用原始函数并插入你的逻辑
// ============================================================================

#pragma once

#include "Base.h"

// ============================================================================
// 核心 Hook 安装函数
// ============================================================================
// 返回 true 表示 Hook 安装成功，false 表示失败。
// 安装成功后，游戏的 Present 和 ResizeBuffers 调用会被拦截。

bool DX11HOOK();

// ============================================================================
// 自定义 Present 函数（替代游戏原始 Present）
// ============================================================================
// 每帧被调用一次。在此函数中：
//   1. 首次调用时初始化 ImGui（Win32 + DX11 后端）
//   2. 检测 INSERT 键切换菜单开关
//   3. 渲染 ImGui 菜单
//   4. 调用原始 Present 保持游戏渲染正常

HRESULT WINAPI hkPresent(IDXGISwapChain* This, UINT SyncInterval, UINT Flags);

// ============================================================================
// 自定义 ResizeBuffers 函数（替代游戏原始 ResizeBuffers）
// ============================================================================
// 窗口大小改变时被调用。需要清理旧的 RenderTarget，
// 调用原始 ResizeBuffers，然后重建渲染资源。

HRESULT WINAPI hkResize(IDXGISwapChain* This, UINT BufferCount, UINT Width, UINT Height,
                        DXGI_FORMAT NewFormat, UINT SwapChainFlags);

// ============================================================================
// 自定义窗口过程（替代游戏原始 WndProc）
// ============================================================================
// 拦截窗口消息，当菜单打开时将输入优先发给 ImGui，
// 若 ImGui 不消费则转发给游戏原始窗口过程。

LRESULT WINAPI hkWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
