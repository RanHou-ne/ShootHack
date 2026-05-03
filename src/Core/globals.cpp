// ============================================================================
// 文件：src/Core/Globals.cpp
// 模块：核心层 - 全局变量定义与生命周期管理
// ============================================================================
// 【用途】
//   所有 extern 声明的全局变量的实际定义（分配存储空间）。
//   同时提供 InitializeGlobals() 和 CleanupGlobals() 管理全局状态的生命周期。
//
// 【调用时机】
//   InitializeGlobals()  → DX11 Hook 安装后立即调用
//   CleanupGlobals()     → DLL 卸载或 Reset 时调用
// ============================================================================

#include "Globals.h"

// ============================================================================
// DirectX 资源
// ============================================================================

ID3D11Device*            g_pd3dDevice           = nullptr;
ID3D11DeviceContext*     g_pd3dDeviceContext    = nullptr;
IDXGISwapChain*          g_pSwapChain           = nullptr;
ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;
HWND                     g_hWnd                 = nullptr;

// ============================================================================
// Hook 相关
// ============================================================================

DWORD64*  g_vtableSwapChain = nullptr;
bool      g_hookInstalled   = false;

PresentFn g_oPresent = nullptr;
WndProcFn g_oWndProc = nullptr;
ResizeFn  g_oResize  = nullptr;

// ============================================================================
// ImGui 状态
// ============================================================================

ImGuiState g_imguiState = {};

// ============================================================================
// 菜单状态
// ============================================================================

MenuOptions g_menuOptions = {};

// ============================================================================
// 配置路径
// ============================================================================

const char* g_configPath = ".\\config\\settings.json";

// ============================================================================
// InitializeGlobals() —— 全局状态初始化
// ============================================================================
// 在所有 Hook 安装完成后调用一次。重置菜单和 ImGui 状态为默认值。
// 注意：不初始化 DirectX 资源，这些由 DX11Hook.cpp 在运行时设置。

void InitializeGlobals()
{
    // ---- 菜单状态默认值 ----
    g_menuOptions.isMenuOpen    = false;
    g_menuOptions.currentTab    = 0;
    g_menuOptions.menuWidth     = 600.0f;
    g_menuOptions.menuHeight    = 400.0f;
    g_menuOptions.menuPosX      = 100.0f;
    g_menuOptions.menuPosY      = 100.0f;
    g_menuOptions.useCustomFont = true;
    g_menuOptions.fontSize      = 16.0f;

    // ---- ImGui 状态默认值 ----
    g_imguiState.imguiInitialized       = false;
    g_imguiState.dx11BackendInitialized = false;

    printf("[+] Globals 初始化完成\n");
}

// ============================================================================
// CleanupGlobals() —— 全局状态清理
// ============================================================================
// 释放所有 DirectX COM 对象。调用后相关指针置为 nullptr。
// 注意：释放顺序与创建顺序相反（先释放 RTV，再释放 Context，最后释放 Device）。

void CleanupGlobals()
{
    // 1) 先释放 RenderTargetView（依赖于 Device）
    if (g_mainRenderTargetView != nullptr)
    {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
    }

    // 2) 再释放 DeviceContext（依赖于 Device）
    if (g_pd3dDeviceContext != nullptr)
    {
        g_pd3dDeviceContext->Release();
        g_pd3dDeviceContext = nullptr;
    }

    // 3) 最后释放 Device
    if (g_pd3dDevice != nullptr)
    {
        g_pd3dDevice->Release();
        g_pd3dDevice = nullptr;
    }

    printf("[+] Globals 清理完成\n");
}

