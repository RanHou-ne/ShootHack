#include "Draw.h"


//// ?????????????????????????? ImGui::GetForegroundDrawList()->AddLine ?????????
//void DrawLine(const Vector2& start, const Vector2& end, ImColor color)
//{
//    // ??? ImGui ??????????��?????????????????????????????
//    ImGui::GetForegroundDrawList()->AddLine({ start.X, start.Y }, { end.X, end.Y }, color);
//}
//
//// ??????��????????
//void DrawRect(const Vector2& topLeft, const Vector2& bottomRight, ImColor color, float thickness = 1.0f)
//{
//    // ??? ImGui ??????????��??????????��???????????????????????????
//    ImGui::GetForegroundDrawList()->AddRect({ topLeft.X, topLeft.Y }, { bottomRight.X, bottomRight.Y }, color, 0, ImDrawFlags_RoundCornersNone, thickness);
//}
//
//// ??????��????????
//void DrawCircle(const Vector2& center, float radius, ImColor color, int numSegments = 32, float thickness = 1.0f)
//{
//    // ??? ImGui ??????????��?????????��??????????????????????????????
//    ImGui::GetForegroundDrawList()->AddCircle({ center.X, center.Y }, radius, color, numSegments, thickness);
//}


Popup::Popup() : visible(false), popupText(nullptr), popupColor(IM_COL32(200, 200, 200, 255)), popupTextColor(IM_COL32(0, 0, 0,255)), saveTime(0.0f) {}

Popup::~Popup() {}

void Popup::Show(const char* text, ImU32 color, ImU32 textColor)
{
    visible = true;
    popupText = text;
    popupColor = color;
    popupTextColor = textColor;
    saveTime = ImGui::GetTime();
}

void Popup::Hide()
{
    visible = false;
}

void Popup::DrawPopup()
{
    if (visible && ImGui::GetTime() - saveTime < 5.0f)
    {
        ImVec2 textSize = ImGui::CalcTextSize(popupText);
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 contentRegionAvail = ImGui::GetContentRegionAvail();

        // 绘制弹窗背景
        ImDrawList* draw = ImGui::GetWindowDrawList();
        draw->AddRectFilled(ImVec2(windowPos.x + contentRegionAvail.x / 2 - textSize.x / 2 - 5, windowPos.y + ImGui::GetWindowSize().y - 50), ImVec2(windowPos.x + contentRegionAvail.x / 2 + textSize.x / 2 + 5, windowPos.y + ImGui::GetWindowSize().y - 20), popupColor, 5.f);

        // 使用 AddText 绘制文本
        draw->AddText(ImVec2(windowPos.x + contentRegionAvail.x / 2 - textSize.x / 2, windowPos.y + ImGui::GetWindowSize().y - 40), popupTextColor, popupText);
    }
}