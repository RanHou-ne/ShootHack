// ============================================================================
// 文件：src/Core/Base.h
// 模块：核心层 - 全局公共头文件
// ============================================================================
// 【用途】
//   项目的全局基础头文件。所有 .cpp 模块通过包含此文件获取：
//   - Windows API 头文件 (Windows.h, d3d11.h, dxgi.h 等)
//   - C/C++ 标准库常用头文件
//   - 常用的链接库声明
//
// 【设计原则】
//   - 单一入口：每个 .cpp 只需 `#include "Base.h"` 即可获得全部系统依赖
//   - 编译优化：使用 WIN32_LEAN_AND_MEAN 精简 Windows 头文件
//   - 命名空间：不使用 `using namespace std`，避免全局污染
//
// 【扩展指南】
//   如需新增全局系统依赖（如 d3d12.h、vulkan.h），在此文件中添加即可，
//   无需修改每个 .cpp 文件的 include。
// ============================================================================

#ifndef SHOOTHACK_BASE_H
#define SHOOTHACK_BASE_H

// ---- 精简 Windows 头文件，加速编译 ----
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// ---- Windows API ----
#include <windows.h>
#include <windowsx.h>

// ---- DirectX 11 ----
#include <d3d11.h>
#include <dxgi.h>
#include <dxgi1_2.h>

// ---- WIC (Windows Imaging Component) 用于纹理加载 ----
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")

// ---- DirectX 链接库 ----
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

// ---- C 标准库 ----
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cfloat>
#include <cstdint>
#include <cinttypes>

// ---- C++ 标准库 ----
#include <string>
#include <vector>
#include <map>
#include <array>
#include <algorithm>
#include <functional>
#include <memory>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <exception>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>

#endif // SHOOTHACK_BASE_H


