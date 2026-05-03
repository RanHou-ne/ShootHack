#pragma once
#include "include/Base.h"   

namespace Hook
{
    // 初始化所有 Hook（包括 Present Hook 和 ImGui）
    bool InitializeAllHooks();

    // 卸载所有 Hook（游戏退出或 DLL 卸载时调用）
    void ShutdownAllHooks();
}