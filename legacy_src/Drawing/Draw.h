#ifndef DRAW_H
#define DRAW_H

#include "imgui/imgui.h"
#include "include/struct.h"

// ==================== 绘制工具函数 ====================

// DrawUtils：对 ImGui DrawList 的轻量封装。
// 适用场景：
// - 需要在菜单窗口或前景层快速绘制基础图元
// - 统一颜色类型转换，减少业务代码里重复拼 ImU32
class DrawUtils
{
public:
    // 绘制线条。
    // start/end: 屏幕坐标
    // color: IM_COL32(...) 或 ColorToImU32 的结果
    // thickness: 线宽
    static void DrawLine(const Vector2& start, const Vector2& end, ImU32 color, float thickness = 1.0f);

    // 绘制矩形。
    // topLeft/bottomRight: 左上/右下屏幕坐标
    // filled=true 时绘制实心矩形，否则绘制边框矩形
    static void DrawRect(const Vector2& topLeft, const Vector2& bottomRight, ImU32 color, float thickness = 1.0f, bool filled = false);

    // 绘制圆形。
    // numSegments 越大越圆滑，性能开销越高。
    static void DrawCircle(const Vector2& center, float radius, ImU32 color, int numSegments = 32, float thickness = 1.0f);

    // 绘制文本。
    // font 可为空；为空时使用 ImGui 当前字体。
    static void DrawText(const Vector2& pos, const char* text, ImU32 color, ImFont* font = nullptr);

    // 绘制带背景文本（常用于提示标签、简易信息框）。
    static void DrawTextWithBackground(const Vector2& pos, const char* text, ImU32 textColor, ImU32 bgColor, ImFont* font = nullptr);

    // 颜色格式转换：自定义 RGBA <-> ImGui 的 ImU32。
    static ImU32 ColorToImU32(const ImColor_RGBA& color);
    static ImColor_RGBA ImU32ToColor(ImU32 imcolor);
};

// ==================== 通知弹窗类 ====================

// Notification：右下角轻量提示。
// 使用步骤：
// 1) Show("文本", 颜色, 持续秒数)
// 2) 每帧先 Update() 再 Draw()
class Notification
{
public:
    Notification();
    ~Notification();

    // 显示提示。
    // duration: 持续时间（秒）
    void Show(const char* text, ImU32 color, float duration = 3.0f);
    // 立即隐藏。
    void Hide();
    // 每帧更新计时状态。
    void Update();
    // 每帧绘制提示（可见时）。
    void Draw();

    bool IsVisible() const { return m_visible; }

private:
    bool m_visible = false;
    const char* m_text = nullptr;
    ImU32 m_color = IM_COL32(255, 255, 255, 255);
    float m_duration = 3.0f;
    float m_elapsedTime = 0.0f;
};

#endif // DRAW_H
