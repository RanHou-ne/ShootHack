#include "menu.h"
#include "engine.h"
#include "aimbot.h"

Options Options;

void Aim_bot01()
{
    ImGui::Checkbox("开启自瞄", &Options.OpenAimbot);
    ImGui::Checkbox("骨骼自瞄", &Options.BoneOn);
    ImGui::Checkbox("静默自瞄", &Options.MemoryAimbot);
    ImGui::Checkbox("穿透自瞄", &Options.Missed_shot);
    ImGui::SliderFloat("自瞄范围", &Options.Aim_Range, 1.f, 500.f, "%.1f");
    ImGui::SliderFloat("XY阈值", &Options.threshold, 0.f, 30.f, "%.1f");