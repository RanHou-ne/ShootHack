#include "globals.h"
#include <cstdio>

// ==================== DirectX 资源 ====================

ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
HWND g_hWnd = nullptr;

// ==================== Hook 相关 ====================

DWORD64* g_vtableSwapChain = nullptr;
bool g_hookInstalled = false;

Present g_oPresent = nullptr;
WndProc g_oWndProc = nullptr;
Resize g_oResize = nullptr;

// ==================== ImGui 状态 ====================

ImGuiState g_imguiState = {};

// ==================== 菜单状态 ====================

MenuOptions g_menuOptions = {};

// ==================== 配置路径 ====================

const char* g_configPath = ".\\config\\settings.json";

// ==================== 初始化函数 ====================

void InitializeGlobals()
{
    g_menuOptions.isMenuOpen = false;
    g_menuOptions.currentTab = 0;
    g_menuOptions.menuWidth = 600.0f;
    g_menuOptions.menuHeight = 400.0f;
    g_menuOptions.menuPosX = 100.0f;
    g_menuOptions.menuPosY = 100.0f;
    g_menuOptions.useCustomFont = true;
    g_menuOptions.fontSize = 16.0f;

    g_imguiState.imguiInitialized = false;
    g_imguiState.dx11BackendInitialized = false;

    printf("[+] Globals initialized\n");
}

void CleanupGlobals()
{
    if (g_mainRenderTargetView != nullptr)
    {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
    }

    if (g_pd3dDeviceContext != nullptr)
    {
        g_pd3dDeviceContext->Release();
        g_pd3dDeviceContext = nullptr;
    }

    if (g_pd3dDevice != nullptr)
    {
        g_pd3dDevice->Release();
        g_pd3dDevice = nullptr;
    }

    printf("[+] Globals cleaned up\n");
}
