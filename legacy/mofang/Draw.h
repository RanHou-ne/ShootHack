#include "imgui/imgui.h"

#ifndef DRAW_H
#define DRAW_H
//
//void DrawLine(const Vector2& start, const Vector2& end, ImColor color);
//
//// 绘制矩形框函数声明
//void DrawRect(const Vector2& topLeft, const Vector2& bottomRight, ImColor color, float thickness = 1.0f);
//
//// 绘制圆形框函数声明
//void DrawCircle(const Vector2& center, float radius, ImColor color, int numSegments = 32, float thickness = 1.0f);
//



class Popup
{
public:
    Popup();
    ~Popup();

    void Show(const char* text, ImU32 color, ImU32 textColor);
    void Hide();

    
    void DrawPopup();  // 公共绘制函数

private:
    bool visible;
    const char* popupText;
    ImU32 popupColor;

    ImU32 popupTextColor;
    float saveTime;

};


#endif // !DRAW_H