// ============================================================================
// 文件：src/Core/Globals.h
// 模块：核心层 - 全局变量声明
// ============================================================================
// 【用途】
//   声明所有跨模块共享的全局变量。所有 .cpp 文件中如需访问全局状态，
//   只需 `#include "Globals.h"` 即可使用 extern 声明的变量。
//
// 【设计原则】
//   - 声明与定义分离：此文件只包含 `extern` 声明，
//     实际定义在 Globals.cpp 中，避免多重定义链接错误。
//   - 集中管理：所有全局状态集中在一处，便于查找和审查。
//   - 命名约定：全局变量以 `g_` 前缀命名（如 g_pd3dDevice）。
//
// 【全局变量清单】
//   类别          变量名                    用途
//   ─────────────────────────────────────────────────────────
//   DirectX       g_pd3dDevice              D3D11 设备指针
//                 g_pd3dDeviceContext        D3D11 设备上下文
//                 g_pSwapChain               游戏 SwapChain
//                 g_mainRenderTargetView     渲染目标视图
//                 g_hWnd                     游戏主窗口句柄
//   Hook          g_vtableSwapChain          SwapChain 虚表地址
//                 g_hookInstalled            Hook 是否已安装
//                 g_oPresent                 原始 Present 函数指针
//                 g_oWndProc                 原始窗口过程
//                 g_oResize                  原始 ResizeBuffers 函数指针
//   ImGui 状态    g_imguiState              ImGui 初始化状态
//   菜单状态      g_menuOptions             菜单窗口选项
//   配置路径      g_configPath              配置文件存放路径
//
// 【扩展指南】
//   添加新的全局变量：
//   1. 在本文件中添加 `extern Type g_variableName;` 声明
//   2. 在 Globals.cpp 中添加 `Type g_variableName = initialValue;` 定义
//   3. 在 InitializeGlobals() 中添加初始化逻辑（如需要）
//   4. 在 CleanupGlobals() 中添加清理逻辑（如需要）
// ============================================================================

#ifndef SHOOTHACK_GLOBALS_H
#define SHOOTHACK_GLOBALS_H

#include "Struct.h"

// ============================================================================
// DirectX 资源指针
// ============================================================================
// 由 DX11Hook.cpp 在 Hook Present 时获取并赋值

extern ID3D11Device*            g_pd3dDevice;           // D3D11 设备
extern ID3D11DeviceContext*     g_pd3dDeviceContext;    // D3D11 设备上下文
extern IDXGISwapChain*          g_pSwapChain;           // 游戏 SwapChain
extern ID3D11RenderTargetView*  g_mainRenderTargetView; // 渲染目标视图
extern HWND                     g_hWnd;                 // 游戏主窗口句柄

// ============================================================================
// Hook 相关
// ============================================================================
// 由 DX11Hook.cpp 在 DX11HOOK() 中设置

extern DWORD64* g_vtableSwapChain;   // SwapChain 虚函数表地址（用于 Hook）
extern bool     g_hookInstalled;     // Hook 是否已安装（防重复安装）

// ---- Present / WndProc / ResizeBuffers 类型别名 ----
typedef HRESULT (WINAPI* PresentFn)(IDXGISwapChain* This, UINT SyncInterval, UINT Flags);
typedef LRESULT  (WINAPI* WndProcFn)(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
typedef HRESULT (WINAPI* ResizeFn) (IDXGISwapChain* This, UINT buffer_count, UINT width, UINT height, DXGI_FORMAT format, UINT flags);

extern PresentFn g_oPresent;    // 原始 Present 函数指针
extern WndProcFn g_oWndProc;    // 原始窗口过程
extern ResizeFn  g_oResize;     // 原始 ResizeBuffers 函数指针

// ============================================================================
// ImGui 状态
// ============================================================================
// 由 DX11Hook.cpp 在 hkPresent 初始化时写入，
// Menu.cpp 和 Renderer.cpp 读取以判断渲染是否就绪

extern ImGuiState g_imguiState;

// ============================================================================
// 菜单窗口状态
// ============================================================================
// 由 Menu.cpp 和 DX11Hook.cpp 共同维护

extern MenuOptions g_menuOptions;

// ============================================================================
// 配置路径
// ============================================================================

extern const char* g_configPath;   // 配置文件存放目录路径

// ============================================================================
// 全局初始化 / 清理
// ============================================================================
// 在 DX11 Hook 安装后、ImGui 初始化前调用 InitializeGlobals()。
// 在 DLL 卸载或 Hook 卸载时调用 CleanupGlobals()。

void InitializeGlobals();
void CleanupGlobals();

#endif // SHOOTHACK_GLOBALS_H

