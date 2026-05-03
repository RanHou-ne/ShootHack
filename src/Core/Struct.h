// ============================================================================
// 文件：src/Core/Struct.h
// 模块：核心层 - 基础数据结构定义
// ============================================================================
// 【用途】
//   定义项目中所有基础数学结构（向量、颜色）和全局状态结构体。
//   所有模块都依赖这些类型进行数据传递和状态管理。
//
// 【结构体清单】
//   Vector2      - 2D 向量（屏幕坐标、窗口位置等）
//   Vector3      - 3D 向量（世界坐标、方向等）
//   Vector4      - 4D 向量（齐次坐标、颜色 RGBA 等）
//   ImColor_RGBA - RGBA 颜色（范围 0.0~1.0）
//   MenuOptions  - 菜单窗口状态（位置、大小、当前Tab）
//   ImGuiState   - ImGui 初始化状态跟踪
//
// 【扩展指南】
//   要添加新的通用数据结构（如 Matrix4x4、BoundingBox 等）：
//   1. 在此文件中定义结构体
//   2. 添加合适的构造函数和运算符重载
//   3. 在需要使用的模块中包含此文件即可
// ============================================================================

#ifndef SHOOTHACK_TYPES_H
#define SHOOTHACK_TYPES_H

#include "Base.h"
#include "ImGui/imgui.h"

// ============================================================================
// 2D 向量
// ============================================================================
// 常用于：屏幕坐标、窗口位置、UI 元素定位
struct Vector2
{
    // 注意：X 和 Y 的顺序与 ImVec2 相反，以便与 Unity 的 Vector2 兼容
    float X = 0.0f;
    float Y = 0.0f;

    // 默认构造函数，避免 ImVec2 的默认构造函数导致的内存泄漏
    Vector2() = default;
    Vector2(float x, float y) : X(x), Y(y) {}

    // 运算符重载
    Vector2 operator+(const Vector2& other) const { return Vector2(X + other.X, Y + other.Y); }
    Vector2 operator-(const Vector2& other) const { return Vector2(X - other.X, Y - other.Y); }
    Vector2 operator*(float scalar)       const { return Vector2(X * scalar, Y * scalar); }

    
    float Distance(const Vector2& other) const
    {
        const float dx = X - other.X;
        const float dy = Y - other.Y;
        return std::sqrt(dx * dx + dy * dy);
    }
};

// ============================================================================
// 3D 向量
// ============================================================================
// 常用于：世界坐标、速度向量、骨骼位置
struct Vector3 : public Vector2
{
    float Z = 0.0f;

    Vector3() = default;
    Vector3(float x, float y, float z) : Vector2(x, y), Z(z) {}

    Vector3 operator+(const Vector3& other) const { return Vector3(X + other.X, Y + other.Y, Z + other.Z); }
    Vector3 operator-(const Vector3& other) const { return Vector3(X - other.X, Y - other.Y, Z - other.Z); }
    Vector3 operator*(float scalar)       const { return Vector3(X * scalar, Y * scalar, Z * scalar); }

    float Distance(const Vector3& other) const
    {
        const float dx = X - other.X;
        const float dy = Y - other.Y;
        const float dz = Z - other.Z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    float Length() const { return std::sqrt(X * X + Y * Y + Z * Z); }
};

// ============================================================================
// 4D 向量
// ============================================================================
// 常用于：齐次坐标变换、RGBA 颜色
struct Vector4 : public Vector3
{
    float W = 0.0f;

    Vector4() = default;
    Vector4(float x, float y, float z, float w) : Vector3(x, y, z), W(w) {}
};

// ============================================================================
// RGBA 颜色结构
// ============================================================================
// 四种分量范围均为 0.0 ~ 1.0，与 ImGui 颜色系统兼容
struct ImColor_RGBA
{
    float R = 1.0f;
    float G = 1.0f;
    float B = 1.0f;
    float A = 1.0f;

    ImColor_RGBA() = default;
    ImColor_RGBA(float r, float g, float b, float a = 1.0f) : R(r), G(g), B(b), A(a) {}
};

// ============================================================================
// 菜单窗口状态
// ============================================================================
// 记录菜单窗口的显示状态、位置、大小等信息。
// 由 Globals 层持有实例，Menu 模块读写。
struct MenuOptions
{
    bool  isMenuOpen    = false;   // 菜单是否可见
    int   currentTab    = 0;       // 当前选中的 Tab 索引
    float menuWidth     = 600.0f;  // 菜单窗口宽度（像素）
    float menuHeight    = 400.0f;  // 菜单窗口高度（像素）
    float menuPosX      = 100.0f;  // 菜单窗口 X 坐标
    float menuPosY      = 100.0f;  // 菜单窗口 Y 坐标
    bool  useCustomFont = true;    // 是否使用自定义字体
    float fontSize      = 16.0f;   // 字体大小
};

// ============================================================================
// ImGui 初始化状态
// ============================================================================
// 跟踪 ImGui 各个子系统的初始化状态，防止重复初始化。
// 由 DX11Hook 模块写入，Menu 模块读取。
struct ImGuiState
{
    ImFont* fontDefault            = nullptr;  // 默认字体
    ImFont* fontMonospace          = nullptr;  // 等宽字体（预留）
    bool    imguiInitialized       = false;    // ImGui 上下文是否已创建
    bool    dx11BackendInitialized = false;    // DX11 渲染后端是否已初始化
};

// ============================================================================
// 4x4 矩阵
// ============================================================================
// 常用于：世界矩阵、视图矩阵、投影矩阵、WorldToScreen 变换
struct D3DXMATRIX
{
    FLOAT _11; FLOAT _12; FLOAT _13; FLOAT _14;
    FLOAT _21; FLOAT _22; FLOAT _23; FLOAT _24;
    FLOAT _31; FLOAT _32; FLOAT _33; FLOAT _34;
    FLOAT _41; FLOAT _42; FLOAT _43; FLOAT _44;
};

#endif // SHOOTHACK_TYPES_H
