#ifndef STRUCT_H
#define STRUCT_H

#include <cmath>
#include <d3d11.h>
#include <imgui/imgui.h>

// ==================== 基础数学结构 ====================

struct Vector2
{
    float X = 0.0f;
    float Y = 0.0f;

    Vector2() = default;
    Vector2(float x, float y) : X(x), Y(y) {}

    Vector2 operator+(const Vector2& other) const
    {
        return Vector2(X + other.X, Y + other.Y);
    }

    Vector2 operator-(const Vector2& other) const
    {
        return Vector2(X - other.X, Y - other.Y);
    }

    Vector2 operator*(float scalar) const
    {
        return Vector2(X * scalar, Y * scalar);
    }

    float Distance(const Vector2& other) const
    {
        float dx = X - other.X;
        float dy = Y - other.Y;
        return std::sqrt(dx * dx + dy * dy);
    }
};

struct Vector3 : public Vector2
{
    float Z = 0.0f;

    Vector3() = default;
    Vector3(float x, float y, float z) : Vector2(x, y), Z(z) {}

    Vector3 operator+(const Vector3& other) const
    {
        return Vector3(X + other.X, Y + other.Y, Z + other.Z);
    }

    Vector3 operator-(const Vector3& other) const
    {
        return Vector3(X - other.X, Y - other.Y, Z - other.Z);
    }

    Vector3 operator*(float scalar) const
    {
        return Vector3(X * scalar, Y * scalar, Z * scalar);
    }

    float Distance(const Vector3& other) const
    {
        float dx = X - other.X;
        float dy = Y - other.Y;
        float dz = Z - other.Z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    float Length() const
    {
        return std::sqrt(X * X + Y * Y + Z * Z);
    }
};

struct Vector4 : public Vector3
{
    float W = 0.0f;

    Vector4() = default;
    Vector4(float x, float y, float z, float w) : Vector3(x, y, z), W(w) {}
};

// ==================== 颜色结构 ====================

struct ImColor_RGBA
{
    float R = 1.0f;
    float G = 1.0f;
    float B = 1.0f;
    float A = 1.0f;

    ImColor_RGBA() = default;
    ImColor_RGBA(float r, float g, float b, float a = 1.0f)
        : R(r), G(g), B(b), A(a) {}
};

// ==================== UI 菜单状态结构 ====================

struct MenuOptions
{
    // 菜单显示状态
    bool isMenuOpen = false;
    int currentTab = 0;
    float menuWidth = 600.0f;
    float menuHeight = 400.0f;
    float menuPosX = 100.0f;
    float menuPosY = 100.0f;

    // 字体
    bool useCustomFont = true;
    float fontSize = 16.0f;
};

// ==================== ImGui 相关 ====================

struct ImGuiState
{
    ImFont* fontDefault = nullptr;
    ImFont* fontMonospace = nullptr;
    bool imguiInitialized = false;
    bool dx11BackendInitialized = false;
};

#endif // STRUCT_H
