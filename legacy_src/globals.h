#ifndef GLOBALS_H
#define GLOBALS_H

#include "include/struct.h"
#include <d3d11.h>
#include <dxgi.h>

// ==================== DirectX 资源 ====================

extern ID3D11Device* g_pd3dDevice;
extern ID3D11DeviceContext* g_pd3dDeviceContext;
extern IDXGISwapChain* g_pSwapChain;
extern ID3D11RenderTargetView* g_mainRenderTargetView;
extern HWND g_hWnd;

// ==================== Hook 相关 ====================

extern DWORD64* g_vtableSwapChain;
extern bool g_hookInstalled;

typedef HRESULT(WINAPI* Present)(IDXGISwapChain* This, UINT SyncInterval, UINT Flags);
typedef LRESULT(WINAPI* WndProc)(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
typedef HRESULT(WINAPI* Resize)(IDXGISwapChain* This, UINT buffer_count, UINT width, UINT height, DXGI_FORMAT format, UINT flags);

extern Present g_oPresent;
extern WndProc g_oWndProc;
extern Resize g_oResize;

// ==================== ImGui 状态 ====================

extern ImGuiState g_imguiState;

// ==================== 菜单状态 ====================

extern MenuOptions g_menuOptions;

// ==================== 配置路径 ====================

extern const char* g_configPath;

// ==================== 初始化函数 ====================

void InitializeGlobals();
void CleanupGlobals();

#endif // GLOBALS_H
