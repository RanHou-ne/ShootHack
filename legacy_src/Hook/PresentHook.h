#pragma once
#include <d3d11.h>

namespace PresentHook
{
    // 初始化 DX11 Hook（包含虚表 Hook + ImGui 初始化）
    bool Initialize();

    // 卸载 Hook
    void Shutdown();

    // 获取游戏的 SwapChain 指针（供外部使用）
    IDXGISwapChain* GetSwapChain();
}