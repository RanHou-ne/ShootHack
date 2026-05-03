#include "Draw.h"
#include "ImGui/imgui_internal.h"

// ==================== DrawUtils 实现 ====================

void DrawUtils::DrawLine(const Vector2& start, const Vector2& end, ImU32 color, float thickness)
{
    // 使用当前窗口 DrawList 绘制；若在非窗口上下文中调用会返回空指针。
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    if (draw_list == nullptr)
        return;

    draw_list->AddLine(ImVec2(start.X, start.Y), ImVec2(end.X, end.Y), color, thickness);
}

void DrawUtils::DrawRect(const Vector2& topLeft, const Vector2& bottomRight, ImU32 color, float thickness, bool filled)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    if (draw_list == nullptr)
        return;

    if (filled)
    {
        // 实心矩形常用于背景块、命中框填充。
        draw_list->AddRectFilled(
            ImVec2(topLeft.X, topLeft.Y),
            ImVec2(bottomRight.X, bottomRight.Y),
            color
        );
    }
    else
    {
        // 空心矩形常用于 ESP 边框。
        draw_list->AddRect(
            ImVec2(topLeft.X, topLeft.Y),
            ImVec2(bottomRight.X, bottomRight.Y),
            color,
            0.0f,
            ImDrawFlags_RoundCornersAll,
            thickness
        );
    }
}

void DrawUtils::DrawCircle(const Vector2& center, float radius, ImU32 color, int numSegments, float thickness)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    if (draw_list == nullptr)
        return;

    draw_list->AddCircle(ImVec2(center.X, center.Y), radius, color, numSegments, thickness);
}

void DrawUtils::DrawText(const Vector2& pos, const char* text, ImU32 color, ImFont* font)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    if (draw_list == nullptr)
        return;

    if (font != nullptr)
    {
        // 指定字号统一为 16，可按项目风格改为参数化。
        draw_list->AddText(font, 16.0f, ImVec2(pos.X, pos.Y), color, text);
    }
    else
    {
        draw_list->AddText(ImVec2(pos.X, pos.Y), color, text);
    }
}

void DrawUtils::DrawTextWithBackground(const Vector2& pos, const char* text, ImU32 textColor, ImU32 bgColor, ImFont* font)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    if (draw_list == nullptr)
        return;

    ImVec2 textSize = ImGui::CalcTextSize(text);
    ImVec2 padding(4.0f, 2.0f);

    // 先绘制背景矩形，再覆盖文字，形成“标签”效果。
    draw_list->AddRectFilled(
        ImVec2(pos.X - padding.x, pos.Y - padding.y),
        ImVec2(pos.X + textSize.x + padding.x, pos.Y + textSize.y + padding.y),
        bgColor
    );

    // 最后绘制文本。
    if (font != nullptr)
    {
        draw_list->AddText(font, 16.0f, ImVec2(pos.X, pos.Y), textColor, text);
    }
    else
    {
        draw_list->AddText(ImVec2(pos.X, pos.Y), textColor, text);
    }
}

ImU32 DrawUtils::ColorToImU32(const ImColor_RGBA& color)
{
    // 约定输入范围 [0,1]，转换到 [0,255]。
    return IM_COL32(
        static_cast<int>(color.R * 255),
        static_cast<int>(color.G * 255),
        static_cast<int>(color.B * 255),
        static_cast<int>(color.A * 255)
    );
}

ImColor_RGBA DrawUtils::ImU32ToColor(ImU32 imcolor)
{
    // 按 ImGui 通道位偏移提取 RGBA。
    return ImColor_RGBA(
        ((imcolor >> IM_COL32_R_SHIFT) & 0xFF) / 255.0f,
        ((imcolor >> IM_COL32_G_SHIFT) & 0xFF) / 255.0f,
        ((imcolor >> IM_COL32_B_SHIFT) & 0xFF) / 255.0f,
        ((imcolor >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f
    );
}

// ==================== Notification 实现 ====================

Notification::Notification() {}

Notification::~Notification() {}

void Notification::Show(const char* text, ImU32 color, float duration)
{
    // 重置计时并切换为可见。
    m_text = text;
    m_color = color;
    m_duration = duration;
    m_elapsedTime = 0.0f;
    m_visible = true;
}

void Notification::Hide()
{
    m_visible = false;
}

void Notification::Update()
{
    if (!m_visible)
        return;

    m_elapsedTime += ImGui::GetIO().DeltaTime;
    // 到时自动隐藏。
    if (m_elapsedTime >= m_duration)
    {
        m_visible = false;
    }
}

void Notification::Draw()
{
    if (!m_visible || m_text == nullptr)
        return;

    ImVec2 textSize = ImGui::CalcTextSize(m_text);
    ImVec2 windowSize = ImGui::GetIO().DisplaySize;
    ImVec2 notificationPos(windowSize.x - textSize.x - 30.0f, windowSize.y - textSize.y - 30.0f);

    // 末尾 0.5s 做淡出，避免提示突兀消失。
    float alpha = 1.0f;
    if (m_elapsedTime > m_duration - 0.5f)
    {
        alpha = (m_duration - m_elapsedTime) * 2.0f;
    }

    ImU32 finalColor = (m_color & IM_COL32(255, 255, 255, 0)) | 
                       static_cast<ImU32>(((m_color >> IM_COL32_A_SHIFT) & 0xFF) * alpha) << IM_COL32_A_SHIFT;

    DrawUtils::DrawTextWithBackground(
        Vector2(notificationPos.x, notificationPos.y),
        m_text,
        finalColor,
        IM_COL32(0, 0, 0, 128)
    );
}
