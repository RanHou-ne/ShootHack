
#pragma once

#include "include/Base.h"
#include "Hook/PresentHook.h"   // Present Hook 实现
#include <string>


namespace Engine
{
	// 解析窗口/模块句柄，并绑定全部 IL2CPP 导出 API。
	// 目标进程准备好后调用一次即可。
	void Initialize();

	// 返回缓存的 Unity 主窗口句柄。
	HWND GetHwnd();

	// 返回从 UnityPlayer 内部结构解析得到的 DX11 SwapChain 指针。
	IDXGISwapChain* GetSwapChain();

	
}

// X-macro 声明阶段：
// il2cpp_api.h 里的每一条 DO_API 都会展开为：
//   1) 函数指针类型别名：Name##_t
//   2) 外部变量声明：extern Name
//
// 真正的变量定义和 GetProcAddress 绑定在 Engine.cpp 里完成。
#define DO_API(RetType, Name, Args) \
	using Name##_t = RetType(__stdcall*)Args; \
	extern Name##_t Name;
#include "il2cpp_api.h"
#undef DO_API

