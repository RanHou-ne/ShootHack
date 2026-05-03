#include "Menu.h"
#include "UI/Draw/Draw.h"
#include "MenuSwitches.h"
#include "MenuFeatureStubs.h"
#include "MenuSearch.h"
#include "Config/Config.h"
#include "Engine/Engine.h"
#include "ImGui/imgui_internal.h"
#include <cfloat>
#include <cmath>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <d3d11.h>

// Import ImGui custom settings
// 注意: 此路径依赖项目 vcxproj 中配置的根目录 include path (ShootHack/)，
// 文件实际位于项目根目录的 ImGui/imgui_settings.h，而非 src/ 下。
#include "ImGui/imgui_settings.h"

namespace font
{
    extern ImFont* pPoppinsMedium;
    extern ImFont* pPoppinsMediumLow;
    extern ImFont* pTabIcon;
    extern ImFont* pChicons;
    extern ImFont* pTahomaBold;
    extern ImFont* pTahomaBold2;
}

namespace image
{
    extern ID3D11ShaderResourceView* bg;
    extern ID3D11ShaderResourceView* logo;
    extern ID3D11ShaderResourceView* logo_general;

    extern ID3D11ShaderResourceView* arrow;
    extern ID3D11ShaderResourceView* bell_notify;
    extern ID3D11ShaderResourceView* roll;
}

static int rotation_start_index;
void ImRotateStart()
{
    // 记录当前顶点缓冲起点，后续对“起点之后新增的图元”做旋转。
    rotation_start_index = ImGui::GetWindowDrawList()->VtxBuffer.Size;
}

// ImRotate removed as it is provided by imgui_internal.h

ImVec2 ImRotationCenter()
{
    // 计算本轮新增图元的包围盒中心作为旋转中心。
    ImVec2 l(FLT_MAX, FLT_MAX), u(-FLT_MAX, -FLT_MAX); 
    const auto& buf = ImGui::GetWindowDrawList()->VtxBuffer;
    for (int i = rotation_start_index; i < buf.Size; i++)
        l = ImMin(l, buf[i].pos), u = ImMax(u, buf[i].pos);
    return ImVec2((l.x + u.x) / 2, (l.y + u.y) / 2); 
}

void ImRotateEnd(float rad, ImVec2 center = ImRotationCenter())
{
    // 对 Start 之后新增顶点批量旋转，常用于箭头开合动画。
    float s = sin(rad), c = cos(rad);
    center = ImRotate(center, s, c) - center;
    auto& buf = ImGui::GetWindowDrawList()->VtxBuffer;
    for (int i = rotation_start_index; i < buf.Size; i++)
        buf[i].pos = ImRotate(buf[i].pos, s, c) - center;
}

void Particles()
{
    // 简易粒子背景：随机出生点 + 线性插值下落。
    // 这里只做视觉增强，不参与任何业务状态。
    if (MenuSwitches::g_Toggles.PerformanceMode)
    {
        // 性能模式下直接关闭粒子层，避免持续 CPU/GPU 开销。
        return;
    }

    ImVec2 screen_size = ImGui::GetIO().DisplaySize;
    static ImVec2 partile_pos[100];
    static ImVec2 partile_target_pos[100];
    static float partile_speed[100];
    static float partile_radius[100];

    for (int i = 1; i < 50; i++)
    {
        if (partile_pos[i].x == 0 || partile_pos[i].y == 0)
        {
            partile_pos[i].x = (float)(rand() % (int)screen_size.x + 1);
            partile_pos[i].y = 15.f;
            partile_speed[i] = 1.0f + (float)(rand() % 25);
            partile_radius[i] = (float)(rand() % 4);
            partile_target_pos[i].x = (float)(rand() % (int)screen_size.x);
            partile_target_pos[i].y = screen_size.y * 2;
        }
        partile_pos[i] = ImLerp(partile_pos[i], partile_target_pos[i], ImGui::GetIO().DeltaTime * (partile_speed[i] / 60));
        if (partile_pos[i].y > screen_size.y)
        {
            partile_pos[i].x = 0;
            partile_pos[i].y = 0;
        }
        ImGui::GetWindowDrawList()->AddCircleFilled(partile_pos[i], partile_radius[i], ImColor(71, 226, 67, 255 / 2));
    }
}

namespace Menu
{
    // ==================== 工具提示辅助 ====================

    // 当 ShowTooltips 开启且当前项被悬停时，使用前台绘制列表渲染提示文字。
    // 用法：紧跟在 ImGui 控件之后调用，如 ImGui::Checkbox("xx", &v); MenuTooltip("说明文字");
    static void MenuTooltip(const char* text)
    {
        if (!MenuSwitches::g_Toggles.ShowTooltips || !text || !text[0])
            return;
        if (!ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup |
                                   ImGuiHoveredFlags_AllowWhenOverlapped))
            return;

        // 使用前台绘制列表，保证 tooltip 在所有层之上显示
        ImDrawList* dl = ImGui::GetForegroundDrawList();
        if (!dl) return;

        const float wrapWidth = ImGui::GetFontSize() * 22.0f;
        const ImVec2 pad(8.0f, 5.0f);
        const ImVec2 mousePos = ImGui::GetIO().MousePos;
        const ImVec2 textSize = ImGui::CalcTextSize(text, nullptr, false, wrapWidth);

        // 定位：鼠标右下方偏移 16px
        ImVec2 tipPos(mousePos.x + 16.0f, mousePos.y + 14.0f);
        ImVec2 tipSize(textSize.x + pad.x * 2, textSize.y + pad.y * 2);

        // 防止超出屏幕右侧/底部
        const ImVec2 display = ImGui::GetIO().DisplaySize;
        if (tipPos.x + tipSize.x > display.x - 4.0f)
            tipPos.x = mousePos.x - tipSize.x - 8.0f;
        if (tipPos.y + tipSize.y > display.y - 4.0f)
            tipPos.y = mousePos.y - tipSize.y - 8.0f;

        // 背景（深灰半透明圆角矩形）
        dl->AddRectFilled(tipPos, tipPos + tipSize, IM_COL32(25, 25, 30, 235), 5.0f);
        // 边框
        dl->AddRect(tipPos, tipPos + tipSize, IM_COL32(90, 90, 100, 200), 5.0f);
        // 文字
        dl->AddText(ImGui::GetFont(), ImGui::GetFontSize(),
                     tipPos + pad, IM_COL32(235, 235, 240, 255),
                     text, nullptr, wrapWidth);
    }

    // ==================== 菜单内部状态 ====================

    bool IsMenuOpen = false;

    namespace
    {
        constexpr const char* kConfigFileName = "settings.json";
        constexpr const char* kAutoConfigFileName = "settings_auto.json";
        constexpr const char* kExportLogPath = ".\\config\\menu.log";

        struct MenuState
        {
            std::vector<std::string> logs;
        };

        struct ToastState
        {
            std::string text;
            ImVec4 color = ImVec4(0.95f, 0.95f, 0.95f, 1.0f);
            double expireAt = 0.0;
            bool active = false;
        };

        MenuState g_menuState;
        ToastState g_toastState;

        void PushLog(const char* text)
        {
            // 固定长度日志队列，避免无限增长。
            if (g_menuState.logs.size() >= 8)
                g_menuState.logs.erase(g_menuState.logs.begin());
            g_menuState.logs.emplace_back(text ? text : "");
        }

        void ShowToast(const char* text, const ImVec4& color = ImVec4(0.95f, 0.95f, 0.95f, 1.0f), double durationSeconds = 2.0)
        {
            g_toastState.text = text ? text : "";
            g_toastState.color = color;
            g_toastState.expireAt = ImGui::GetTime() + durationSeconds;
            g_toastState.active = true;
        }

        void DrawToast()
        {
            if (!g_toastState.active)
                return;

            if (ImGui::GetTime() >= g_toastState.expireAt)
            {
                g_toastState.active = false;
                return;
            }

            ImGuiViewport* vp = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(vp->Pos + ImVec2(vp->Size.x - 20.0f, 20.0f), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
            ImGui::SetNextWindowBgAlpha(0.90f);

            if (ImGui::Begin("##MenuToast", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings))
            {
                ImGui::TextColored(g_toastState.color, "%s", g_toastState.text.c_str());
            }
            ImGui::End();
        }

        bool ExportLogsToFile(const std::string& filepath)
        {
            try
            {
                const std::filesystem::path outPath(filepath);
                if (outPath.has_parent_path())
                    std::filesystem::create_directories(outPath.parent_path());

                std::ofstream file(filepath, std::ios::out | std::ios::trunc);
                if (!file)
                    return false;

                for (const auto& line : g_menuState.logs)
                    file << line << "\n";

                return true;
            }
            catch (...)
            {
                return false;
            }
        }
    }


    // ==================== Tab 页面实现 ====================

    void DrawMenuHeader()
    {
        ImGui::TextUnformatted("ShootHack Control Center");
        ImGui::SameLine();
        ImGui::TextDisabled("v1.0 ImGui 1.91+ DX11");
        ImGui::Separator();
    }

    // ================================================================
    // 函数：DrawOverviewTab —— 颜色/外观设置页面（mofang 颜色/Color 页）
    //
    // 【功能说明】
    //   管理所有和颜色相关的配置。用户可以在此页面调整所有 ESP 元素的
    //   显示颜色，包括方框、名字、射线、距离文字、血量条、骨骼等。
    //
    // 【mofang 对照】
    //   对应 mofang/menu.h 中的 Color 函数页面。
    //   每个颜色项使用 ImGui::ColorEdit4 + 颜色拾取器。
    //
    // 【扩展指南】
    //   要添加新的颜色项：
    //   1. 在 ConfigSchema.h 的 CFG_COLOR_ITEMS 中添加 float[4] 条目
    //   2. 在 MenuSwitches.h 中添加对应的 float ColorXXX[4] 字段
    //   3. 在本函数中添加 ImGui::ColorEdit4 控件
    //   4. 在 DrawEspShowPane 或实际渲染中使用新颜色
    // ================================================================
    void DrawOverviewTab()
    {
        ImGui::TextUnformatted("颜色配置 - Colors");
        ImGui::Separator();
        ImGui::Spacing();

        const ImGuiColorEditFlags picker_flags = ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaPreview;

        ImGui::SeparatorText("ESP 颜色");

        ImGui::ColorEdit4("2D方框颜色", MenuSwitches::g_Toggles.ColorBox2D, picker_flags);
        MenuTooltip("2D 方框的描边颜色，仅在方框ESP开启时生效");
        ImGui::ColorEdit4("3D方框颜色", MenuSwitches::g_Toggles.ColorBox3D, picker_flags);
        MenuTooltip("3D 方框的描边颜色");
        ImGui::ColorEdit4("名字颜色", MenuSwitches::g_Toggles.ColorName, picker_flags);
        MenuTooltip("目标名称文字颜色");
        ImGui::ColorEdit4("射线颜色", MenuSwitches::g_Toggles.ColorSnapline, picker_flags);
        MenuTooltip("从屏幕边缘到目标的追踪线颜色");
        ImGui::ColorEdit4("距离颜色", MenuSwitches::g_Toggles.ColorDistance, picker_flags);
        MenuTooltip("距离数字文字颜色");
        ImGui::ColorEdit4("血量颜色", MenuSwitches::g_Toggles.ColorHealth, picker_flags);
        MenuTooltip("血量条填充颜色");
        ImGui::ColorEdit4("可见骨骼颜色", MenuSwitches::g_Toggles.ColorBoneVisible, picker_flags);
        MenuTooltip("未被遮挡的骨骼线颜色（浅色=可见）");
        ImGui::ColorEdit4("遮挡骨骼颜色", MenuSwitches::g_Toggles.ColorBoneOccluded, picker_flags);
        MenuTooltip("被墙壁等遮挡的骨骼线颜色（深色=穿墙）");
        ImGui::ColorEdit4("雷达颜色", MenuSwitches::g_Toggles.ColorRadar, picker_flags);
        MenuTooltip("雷达上敌人标记点的颜色");
        ImGui::ColorEdit4("物品高亮颜色", MenuSwitches::g_Toggles.ColorLoot, picker_flags);
        MenuTooltip("地面可拾取物品的高亮颜色");

        ImGui::Spacing();
        ImGui::SeparatorText("全局设置");

        ImGui::SliderFloat("主题色相", &MenuSwitches::g_Toggles.AccentHue, 0.0f, 1.0f, "%.2f");
        MenuTooltip("调整菜单主题色调 (HSV H通道)，0=红 0.33=绿 0.66=蓝");
        ImGui::SliderFloat("全局透明度", &MenuSwitches::g_Toggles.GlobalAlpha, 0.60f, 1.0f, "%.2f");
        MenuTooltip("所有覆盖层元素的整体透明度，值越低越透明");
        ImGui::Checkbox("动态渐变", &MenuSwitches::g_Toggles.DynamicGradient);
        MenuTooltip("开启后菜单背景/高亮使用动态渐变动画，关闭则静态纯色");

        ImGui::Spacing();
        if (ImGui::Button("应用主题色相", ImVec2(220, 0)))
        {
            const bool ok = MenuFeatureStubs::ApplyThemeConfig(
                MenuSwitches::g_Toggles.AccentHue,
                MenuSwitches::g_Toggles.GlobalAlpha,
                MenuSwitches::g_Toggles.UiScale);
            PushLog(ok ? "主题色相已应用" : MenuFeatureStubs::NotImplementedMessage());
        }
        ImGui::TextDisabled("可扩展方向：主题预设 / 配色导入导出 / 自动跟随系统主题");
    }

    // ================================================================
    // 函数：DrawAimbotTab —— 自瞄/瞄准设置主面板（mofang Aim_bot01 风格）
    //
    // 【功能说明】
    //   本页面集中控制所有与自瞄（Aimbot）相关的功能开关和参数。
    //   包括：自瞄总开关、骨骼瞄准模式、静默自瞄、穿透自瞄、
    //   扳机开火、预测瞄准、FOV 限制、平滑系数等。
    //
    // 【mofang 对照】
    //   对应 mofang/menu.h 中的 Aim_bot01 函数，是瞄准功能的主面板。
    //   分为 "Main" 和 "Setting" 两个逻辑区块：
    //   - Main: 核心开关 + 参数滑条
    //   - Setting: 高级微调选项
    //
    // 【用户指南 - 常用术语解释】
    //   - 自瞄 (Aimbot): 自动将准星对准敌人
    //   - 静默自瞄 (Silent Aim): 不移动准星但子弹命中敌人（服务端验证严格时慎用）
    //   - 穿透自瞄 (Wallbang Aim): 无视障碍物瞄准敌人
    //   - 扳机 (Triggerbot): 准星移到敌人上自动开枪
    //   - FOV (Field of View): 自瞄的有效角度范围，越小越精准
    //   - 平滑 (Smooth): 自瞄移动的平滑度，越高越自然
    //
    // 【扩展指南】
    //   要添加新的自瞄功能：
    //   1. 在 MenuSwitches.h 的 ToggleState 中新增字段
    //   2. 在 MenuSwitches.cpp 的 ResetDefaults 中初始化
    //   3. 在本函数中添加 ImGui 控件
    //   4. 在 MenuFeatureStubs 添加对应的 Apply* 函数
    // ================================================================
    void DrawAimbotTab()
    {
        ImGui::TextUnformatted("自瞄 - Main");  // 对应 mofang Aim_bot01 面板标题
        ImGui::Separator();
        ImGui::Spacing();

        // ---- 核心功能开关 ----

        /* 启用自瞄总开关 (Aimbot Toggle)
         * 打开后自瞄功能开始工作。关闭时所有子功能均不生效。
         * 建议与其他开关配合：先开总开关，再微调各子项。 */
        ImGui::Checkbox("开启自瞄", &MenuSwitches::g_Toggles.OpenAimbot);
        MenuTooltip("自瞄总开关，打开后自瞄功能开始工作，关闭时所有子功能均不生效");

        /* 骨骼瞄准 (Bone Aim)
         * 开启后自瞄会瞄准敌人骨骼位置（如头部/胸部骨骼），
         * 关闭则瞄准敌人中心位置。骨骼瞄准通常更精确。 */
        ImGui::Checkbox("瞄准骨骼", &MenuSwitches::g_Toggles.BoneOn);
        MenuTooltip("瞄准敌人骨骼位置（头/胸等），关闭则瞄准中心位置");

        /* 静默自瞄模式 (Silent Aimbot)
         * 开启后客户端不显示准星移动到敌人身上，但子弹会命中。
         * 注意：部分反作弊会检测此类行为。 */
        ImGui::Checkbox("内存自瞄", &MenuSwitches::g_Toggles.MemoryAimbot);
        MenuTooltip("静默自瞄：不移动准星但子弹命中敌人，注意反作弊检测");

        /* 内存漏打 (Missed_shot)
         * 开启后可以无视任何障碍物击打敌人。
         * 适合穿墙射击场景。 */
        ImGui::Checkbox("内存漏打", &MenuSwitches::g_Toggles.Missed_shot);
        MenuTooltip("穿透自瞄：无视障碍物瞄准敌人，适合穿墙射击");

        ImGui::Spacing();

        // ---- 核心参数滑条 ----

        /* 自瞄最大范围 (Aim Range / Max Distance)
         * 只对在此距离内的敌人触发自瞄。
        */
        ImGui::SliderFloat("自瞄范围", &MenuSwitches::g_Toggles.Aim_Range, 1.f, 500.f, "%.1f");
        MenuTooltip("自瞄最大作用距离（米），超出此距离的敌人不触发自瞄");

        /* XY 偏移阈值 (XY Threshold)
         * 当准星与敌人中心的偏移小于此值时触发自瞄修正。
         * 值越小自瞄越"不灵敏"，避免轻微晃动就锁人。 */
        ImGui::SliderFloat("XY偏移阈值", &MenuSwitches::g_Toggles.threshold, 0.f, 30.f, "%.1f");
        MenuTooltip("准星与目标偏移小于此值时触发修正，越小越不灵敏");

        ImGui::Spacing();
        ImGui::SeparatorText("高级设置 - Advanced");  // 对应 mofang Aim_bot03 面板

        /* 扳机开火 (Triggerbot)
         * 开启后当准星移动到敌人身上时将自动开枪。
         * 通常结合自瞄或 FOV 限制一起使用。 */
        ImGui::Checkbox("扳机开火", &MenuSwitches::g_Toggles.TriggerBot);
        MenuTooltip("准星移到敌人身上时自动开枪，通常结合自瞄或FOV使用");

        /* 预测瞄准 (Aim Prediction)
         * 开启后自瞄会考虑敌人的移动速度和方向，
         * 提前预判敌人位置，适合射击移动目标。 */
        ImGui::Checkbox("预测瞄准", &MenuSwitches::g_Toggles.AimPrediction);
        MenuTooltip("考虑敌人移动速度提前预判位置，适合射击移动目标");

        /* FOV 限制开关 (FOV Limit)
         * 开启后只有敌人在指定角度范围内才触发自瞄。
         * 有助于减少视野边缘的误锁。 */
        ImGui::Checkbox("FOV限制", &MenuSwitches::g_Toggles.AimFovLimit);
        MenuTooltip("只对指定角度范围内的敌人触发自瞄，减少视野边缘误锁");

        /* FOV 角度值 (FOV Angle)
         * 自瞄的有效角度范围（度数）。
         * 以屏幕中心为原点，建议范围 5°-180°。
         * 5°-15°: 极精准/低显性，30°-90°: 中等，180°: 全屏锁 */
        ImGui::SliderFloat("FOV角度", &MenuSwitches::g_Toggles.AimFov, 1.0f, 180.0f, "%.1f°");
        MenuTooltip("自瞄有效角度范围：5-15°极精准，30-90°中等，180°全屏锁");

        /* 平滑系数 (Aim Smoothness)
         * 控制自瞄从当前位置移动到目标的速度。
         * 1 = 瞬间锁定（暴力锁），10+ = 更自然的人类移动感。
         * 过高可能导致追不上快速移动的敌人。 */
        ImGui::SliderFloat("平滑系数", &MenuSwitches::g_Toggles.AimSmooth, 1.0f, 20.0f, "%.1f");
        MenuTooltip("1=瞬间锁定(暴力)，10+=自然移动，过高追不上快速目标");

        /* 骨骼瞄准索引 (Bone Index)
         * 选择自瞄锁定的目标骨骼：
         * 0 = 头部 (Head), 1 = 颈部 (Neck), 2 = 胸部 (Chest) */
        ImGui::SliderInt("骨骼索引(0=头,1=颈,2=胸)", &MenuSwitches::g_Toggles.AimBoneIndex, 0, 2);
        MenuTooltip("0=头部 1=颈部 2=胸部，选择自瞄锁定的骨骼位置");

        ImGui::Spacing();
        // 预留调用：统一提交瞄准参数给游戏引擎
        if (ImGui::Button("提交自瞄参数(预留接口)", ImVec2(220, 0)))
        {
            const bool ok = MenuFeatureStubs::ApplyAimbotConfig(
                MenuSwitches::g_Toggles.OpenAimbot,
                MenuSwitches::g_Toggles.Aim_Range,
                MenuSwitches::g_Toggles.AimSmooth,
                MenuSwitches::g_Toggles.AimBoneIndex);
            PushLog(ok ? "自瞄参数提交成功" : MenuFeatureStubs::NotImplementedMessage());
        }
        ImGui::SameLine();
        if (ImGui::Button("扳机测试(预留)", ImVec2(140, 0)))
        {
            const bool ok = MenuFeatureStubs::TriggerSingleShot();
            PushLog(ok ? "扳机测试触发成功" : MenuFeatureStubs::NotImplementedMessage());
        }
        ImGui::TextDisabled("说明：当前仅展示参数配置界面，功能需接入游戏引擎层");
    }

    // ================================================================
    // 函数：DrawESPTab —— 视觉/透视设置主面板（mofang Esp01 风格）
    //
    // 【功能说明】
    //   本页面集中控制所有 ESP（Extra Sensory Perception）绘制功能。
    //   俗称"透视"，可以在屏幕上显示敌人/物品的位置和信息。
    //
    // 【mofang 对照】
    //   对应 mofang/menu.h 中的 Esp01 函数，是视觉功能的主面板（Main）。
    //   配合右侧的 Show（预览）、Other（样式）、Filter（过滤）面板使用。
    //
    // 【ESP 术语详解】
    //   - Box (方框): 在敌人外围绘制一个矩形框
    //   - Name (名字): 在框上方显示敌人名字
    //   - Health (血量): 用竖条或数字显示敌人血量百分比
    //   - Distance (距离): 显示与敌人的距离（米）
    //   - Snapline (射线/拉线): 从屏幕边缘到敌人画一条辅助线
    //   - Skeleton (骨骼): 绘制敌人内部骨骼结构
    //   - Loot (物资): 显示地面可拾取物品
    //   - Radar (雷达): 小地图显示敌人位置
    //
    // 【扩展指南】
    //   要添加新的 ESP 功能：
    //   1. 在 MenuSwitches.h 的 ToggleState 中新增字段
    //   2. 在 MenuSwitches.cpp 的 ResetDefaults 初始化
    //   3. 在本函数添加 ImGui 控件
    //   4. 在 DrawEspShowPane 中添加预览绘制
    //   5. 在 Hook 层的绘制函数中实现真正渲染
    // ================================================================
    void DrawESPTab()
    {
        ImGui::TextUnformatted("视觉 - Main");  // 对应 mofang Esp01 面板标题
        ImGui::Separator();
        ImGui::Spacing();

        // ---- 核心 ESP 功能开关 ----

        /* 方框 ESP (Box ESP)
         * 在目标周围绘制 2D 矩形框。支持实心/空心/拐角三种样式。
         * 拐角样式：只绘制四个角，视野更清晰不挡视线。 */
        ImGui::Checkbox("方框 ESP", &MenuSwitches::g_Toggles.EspBox);
        MenuTooltip("在目标周围绘制2D矩形框，支持实心/空心/拐角三种样式");

        /* 名字 ESP (Name ESP)
         * 在方框上方显示目标的名称或标签。
         * 颜色在"颜色"页中配置。 */
        ImGui::Checkbox("名字 ESP", &MenuSwitches::g_Toggles.EspName);
        MenuTooltip("在方框上方显示目标名称，颜色在颜色页配置");

        /* 血量 ESP (Health ESP)
         * 在方框左侧显示血量条 + 血量百分比文字。
         * 绿色 = 高血量，黄色 = 中血量，红色 = 低血量。 */
        ImGui::Checkbox("血量 ESP", &MenuSwitches::g_Toggles.EspHealth);
        MenuTooltip("在方框左侧显示血量条，绿=高黄=中红=低");

        /* 距离 ESP (Distance ESP)
         * 在方框下方显示与目标的距离（米）。
         * 可以快速判断哪些敌人威胁最大。 */
        ImGui::Checkbox("距离 ESP", &MenuSwitches::g_Toggles.EspDistance);
        MenuTooltip("在方框下方显示距离(米)，判断威胁大小");

        /* 骨骼绘制 (Skeleton)
         * 绘制目标的内部骨骼连接线。
         * 绿色 = 可见骨骼，紫色 = 被遮挡的骨骼（穿墙透视）。 */
        ImGui::Checkbox("骨骼绘制", &MenuSwitches::g_Toggles.EspSkeleton);
        MenuTooltip("绘制目标骨骼连接线，绿色=可见，紫色=被遮挡");

        /* 射线/拉线 (Snapline)
         * 从屏幕底部/侧边缘到目标画一条辅助追踪线。
         * 颜色在"颜色"页配置。 */
        ImGui::Checkbox("射线绘制", &MenuSwitches::g_Toggles.EspSnapline);
        MenuTooltip("从屏幕边缘到目标画追踪线，颜色在颜色页配置");

        ImGui::Spacing();
        ImGui::SeparatorText("扩展功能");

        /* 物资显示 (Loot ESP)
         * 在场景中标记可拾取物品（武器、弹药、护甲等）。
         * 物品颜色在"颜色"页配置。 */
        ImGui::Checkbox("物资显示", &MenuSwitches::g_Toggles.EspLoot);
        MenuTooltip("在场景中标记可拾取物品（武器、弹药、护甲等）");

        /* 雷达显示 (Radar)
         * 在屏幕角落或自定义位置显示小地图雷达，
         * 标记敌人位置和方向。支持圆形/方形两种样式。 */
        ImGui::Checkbox("雷达显示", &MenuSwitches::g_Toggles.EspRadar);
        MenuTooltip("小地图雷达显示敌人位置和方向");

        ImGui::Spacing();

        // ---- ESP 参数滑条 ----

        /* 最大显示距离 (Max Render Distance)
         * 单位为米。超出此距离的目标不会渲染任何 ESP 信息。
         * 提高性能利用率，建议结合游戏实际情况设置。 */
        ImGui::SliderFloat("最大显示距离", &MenuSwitches::g_Toggles.EspMaxDistance, 30.0f, 500.0f, "%.0f m");
        MenuTooltip("超出此距离的目标不渲染ESP，提高性能");

        /* 方框线条粗细 (ESP Box Thickness)
         * 控制方框边框的像素宽度，1-3 像素视觉效果较好。 */
        ImGui::SliderFloat("方框线条粗细", &MenuSwitches::g_Toggles.EspThickness, 0.5f, 5.0f, "%.1f px");
        MenuTooltip("方框边框像素宽度，1-3像素视觉效果较好");

        /* 文字缩放 (Text Scale)
         * 控制所有 ESP 文字的缩放比例，包括名字、距离、血量百分比。
         * 1.0 = 正常大小，需要更大字可调至 1.5-1.8 */
        //ImGui::SliderFloat("文字缩放", &MenuSwitches::g_Toggles.EspTextScale, 0.6f, 1.8f, "%.2f");

        ImGui::Spacing();
        // 预留调用：统一提交视觉参数
        if (ImGui::Button("提交视觉参数(预留接口)", ImVec2(220, 0)))
        {
            const bool ok = MenuFeatureStubs::ApplyEspConfig(
                MenuSwitches::g_Toggles.EspMaxDistance,
                MenuSwitches::g_Toggles.EspThickness,
                MenuSwitches::g_Toggles.EspTextScale);
            PushLog(ok ? "视觉参数提交成功" : MenuFeatureStubs::NotImplementedMessage());
        }
        ImGui::TextDisabled("建议: 先在 Main 面板开启功能，再到 Show 面板预览效果");
    }

    // 视觉页右上 Show 面板：展示当前开启项的“预览渲染结果”。
static void DrawEspShowPane()
{
    // 获取当前 ImGui 窗口的位置和绘制列表
    ImVec2 pos = ImGui::GetWindowPos();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    
    // ===== 射线绘制预览 =====
    if (MenuSwitches::g_Toggles.EspSnapline)
    {
        ImGui::SetCursorPos(ImVec2(60, 65));
        ImVec2 pos1 = ImGui::GetCursorScreenPos();
        
        // 在方框下方显示 "射线" 文字标签
        ImU32 snaplineColor = ImGui::ColorConvertFloat4ToU32(ImVec4(
            MenuSwitches::g_Toggles.ColorSnapline[0],
            MenuSwitches::g_Toggles.ColorSnapline[1],
            MenuSwitches::g_Toggles.ColorSnapline[2],
            MenuSwitches::g_Toggles.ColorSnapline[3]
        ));
        draw->AddText(ImVec2(pos1.x + 10, pos1.y + 360 + 5), snaplineColor, u8"射线");
    }
    
    // ===== 2D 方框预览 =====
    if (MenuSwitches::g_Toggles.EspBox)
    {
        ImGui::SetCursorPos(ImVec2(50, 15));
        ImVec2 pos1 = ImGui::GetCursorScreenPos();
        
        ImU32 boxColor = ImGui::ColorConvertFloat4ToU32(ImVec4(
            MenuSwitches::g_Toggles.ColorBox2D[0],
            MenuSwitches::g_Toggles.ColorBox2D[1],
            MenuSwitches::g_Toggles.ColorBox2D[2],
            MenuSwitches::g_Toggles.ColorBox2D[3]
        ));
        
        // 绘制方框
        draw->AddRect(
            ImVec2(pos1.x, pos1.y), 
            ImVec2(pos1.x + 200, pos1.y + 360), 
            boxColor, 
            0.0f, 0, 
            MenuSwitches::g_Toggles.EspThickness
        );
        
        // 添加阴影效果
        draw->AddShadowRect(
            ImVec2(pos1.x, pos1.y), 
            ImVec2(pos1.x + 200, pos1.y + 360), 
            ImColor(255, 255, 255, 255), 
            14.0f, 
            ImVec2(0, 0), 0, 0
        );
    }
    
    // ===== 血量条预览 =====
    if (MenuSwitches::g_Toggles.EspHealth)
    {
        ImGui::SetCursorPos(ImVec2(42, 15));
        ImVec2 pos1 = ImGui::GetCursorScreenPos();
        
        ImU32 healthColor = ImGui::ColorConvertFloat4ToU32(ImVec4(
            MenuSwitches::g_Toggles.ColorHealth[0],
            MenuSwitches::g_Toggles.ColorHealth[1],
            MenuSwitches::g_Toggles.ColorHealth[2],
            MenuSwitches::g_Toggles.ColorHealth[3]
        ));
        
        const float hpBarWidth = 4.0f;
        
        // 绘制血量条（从下往上填充，模拟 66% 血量）
        float hpPercent = 0.66f;
        float hpHeight = 360 * hpPercent;
        
        draw->AddRectFilled(
            ImVec2(pos1.x, pos1.y + 360 - hpHeight), 
            ImVec2(pos1.x + hpBarWidth, pos1.y + 360), 
            healthColor
        );
        
        // 添加阴影效果
        draw->AddShadowRect(
            ImVec2(pos1.x, pos1.y), 
            ImVec2(pos1.x + hpBarWidth, pos1.y + 360), 
            healthColor, 
            hpBarWidth, 
            ImVec2(0, 0), 0, 0
        );
    }
    
    // ===== 名字预览 =====
    if (MenuSwitches::g_Toggles.EspName)
    {
        ImGui::SetCursorPos(ImVec2(50, 15));
        ImVec2 pos1 = ImGui::GetCursorScreenPos();
        
        ImU32 nameColor = ImGui::ColorConvertFloat4ToU32(ImVec4(
            MenuSwitches::g_Toggles.ColorName[0],
            MenuSwitches::g_Toggles.ColorName[1],
            MenuSwitches::g_Toggles.ColorName[2],
            MenuSwitches::g_Toggles.ColorName[3]
        ));
        
        // 计算名字居中位置
        const char* nameText = u8"名字";
        float nameWidth = ImGui::CalcTextSize(nameText).x;
        float nameX = pos1.x + 100 - nameWidth / 2;
        float nameY = pos1.y - 16;
        
        draw->AddText(ImVec2(nameX, nameY), nameColor, nameText);
    }
    
    // ===== 距离预览 =====
    if (MenuSwitches::g_Toggles.EspDistance)
    {
        ImGui::SetCursorPos(ImVec2(50, 15));
        ImVec2 pos1 = ImGui::GetCursorScreenPos();
        
        ImU32 distanceColor = ImGui::ColorConvertFloat4ToU32(ImVec4(
            MenuSwitches::g_Toggles.ColorDistance[0],
            MenuSwitches::g_Toggles.ColorDistance[1],
            MenuSwitches::g_Toggles.ColorDistance[2],
            MenuSwitches::g_Toggles.ColorDistance[3]
        ));
        
        // 计算距离文字居中位置
        const char* distanceText = u8"距离";
        float distanceWidth = ImGui::CalcTextSize(distanceText).x;
        float distanceX = pos1.x + 100 - distanceWidth;
        float distanceY = pos1.y + 360 + 8;
        
        draw->AddText(ImVec2(distanceX, distanceY), distanceColor, distanceText);
    }
    
    // ===== 骨骼预览 =====
    if (MenuSwitches::g_Toggles.EspSkeleton)
    {
        ImGui::SetCursorPos(ImVec2(26, 74));
        ImVec2 pos1 = ImGui::GetCursorScreenPos();
        
        ImU32 boneColor = ImGui::ColorConvertFloat4ToU32(ImVec4(
            MenuSwitches::g_Toggles.ColorBoneOccluded[0],
            MenuSwitches::g_Toggles.ColorBoneOccluded[1],
            MenuSwitches::g_Toggles.ColorBoneOccluded[2],
            MenuSwitches::g_Toggles.ColorBoneOccluded[3]
        ));
        
        // 定义骨骼关节位置
        ImVec2 head(122 + pos1.x, pos1.y - 20);           // 头部
        ImVec2 neck(122 + pos1.x, pos1.y);                // 颈部
        ImVec2 shoulderRight(70 + pos1.x, 40 + pos1.y);   // 右肩
        ImVec2 elbowRight(45 + pos1.x, 100 + pos1.y);     // 右肘
        ImVec2 shoulderLeft(170 + pos1.x, 20 + pos1.y);   // 左肩
        ImVec2 elbowLeft(210 + pos1.x, 100 + pos1.y);     // 左肘
        ImVec2 pelvis(121 + pos1.x, 110 + pos1.y);        // 骨盆
        ImVec2 hipRight(80 + pos1.x, 207 + pos1.y);       // 右臀
        ImVec2 kneeRight(80 + pos1.x, 258 + pos1.y);      // 右膝
        ImVec2 hipLeft(175 + pos1.x, 210 + pos1.y);       // 左臀
        ImVec2 kneeLeft(175 + pos1.x, 261 + pos1.y);      // 左膝
        
        const float boneThickness = 1.8f;
        
        // 绘制骨骼连接线
        draw->AddLine(head, neck, boneColor, boneThickness);                    // 头 → 颈
        draw->AddLine(neck, pelvis, boneColor, boneThickness);                  // 颈 → 骨盆
        draw->AddLine(neck, shoulderRight, boneColor, boneThickness);           // 颈 → 右肩
        draw->AddLine(shoulderRight, elbowRight, boneColor, boneThickness);     // 右肩 → 右肘
        draw->AddLine(neck, shoulderLeft, boneColor, boneThickness);            // 颈 → 左肩
        draw->AddLine(shoulderLeft, elbowLeft, boneColor, boneThickness);       // 左肩 → 左肘
        draw->AddLine(pelvis, hipRight, boneColor, boneThickness);              // 骨盆 → 右臀
        draw->AddLine(hipRight, kneeRight, boneColor, boneThickness);           // 右臀 → 右膝
        draw->AddLine(pelvis, hipLeft, boneColor, boneThickness);               // 骨盆 → 左臀
        draw->AddLine(hipLeft, kneeLeft, boneColor, boneThickness);             // 左臀 → 左膝
    }
    
    // 如果没有任何功能启用，显示提示信息
    const bool anyEnabled =
        MenuSwitches::g_Toggles.EspBox ||
        MenuSwitches::g_Toggles.EspName ||
        MenuSwitches::g_Toggles.EspHealth ||
        MenuSwitches::g_Toggles.EspDistance ||
        MenuSwitches::g_Toggles.EspSkeleton ||
        MenuSwitches::g_Toggles.EspSnapline;
    
    if (!anyEnabled)
    {
        ImGui::SetCursorPos(ImVec2(50, 180));
        ImGui::TextDisabled(u8"暂无可预览内容");
        ImGui::SetCursorPos(ImVec2(30, 200));
        ImGui::TextDisabled(u8"请先在 Main 面板启用 ESP 项");
    }
}


    // ================================================================
    // 函数：DrawEspOtherPane —— ESP 样式/Other 面板（mofang Esp03 风格）
    //
    // 【功能说明】
    //   控制 ESP 方框的额外样式选项：
    //   - 实心方框（方框内部半透明填充）
    //   - 拐角方框（只绘制四个角，更简洁）
    //   - 方框颜色快速选择
    //
    // 【扩展指南】
    //   添加新样式：
    //   1. 在 MenuSwitches.h 添加 bool 开关
    //   2. 在本函数添加 ImGui::Checkbox
    //   3. 在 DrawEspShowPane 或实际渲染函数中实现对应样式
    // ================================================================
    static void DrawEspOtherPane()
    {
        ImGui::TextUnformatted("样式 - Other");  // 对应 mofang Esp03 面板
        ImGui::Separator();

        /* 实心方框 (Filled Box)
         * 开启后 ESP 方框内部会以半透明颜色填充。
         * 方便在复杂背景中更容易识别目标。 */
        ImGui::Checkbox("实心方框", &MenuSwitches::g_Toggles.EspFillBox);
        MenuTooltip("方框内部半透明填充，在复杂背景中更容易识别目标");

        /* 拐角方框 (Corner Box)
         * 开启后只绘制方框的四个角，中间线条消失。
         * 视觉更简洁，不遮挡目标身体。 */
        ImGui::Checkbox("拐角方框", &MenuSwitches::g_Toggles.EspCornerBox);
        MenuTooltip("只绘制四个角，视觉更简洁不遮挡目标");

        // /* ESP 快速颜色 (Quick Color)
        //  * 在此处快速调节方框颜色，更详细的颜色配置在"颜色"页面。 */
        // ImGui::ColorEdit4("方框颜色(快速)", MenuSwitches::g_Toggles.EspColor, ImGuiColorEditFlags_NoInputs);

        // ---- 空行留白，占满面板，保持布局稳定 ----
        //ImGui::TextDisabled("颜色详细配置请到 颜色 页面");
    }

    // ================================================================
    // 函数：DrawEspFilterPane —— ESP 过滤/Filter 面板（mofang Esp04 风格）
    //
    // 【功能说明】
    //   控制哪些目标类型显示 ESP 绘制：
    //   - 按类型过滤（僵尸、吸血鬼等）
    //   - 按可见性过滤（仅可见目标、忽略队友等）
    //   - 按状态过滤（排除已死亡目标）
    //
    // 【使用技巧】
    //   合理配置过滤规则有两个好处：
    //   1. 减少画面杂乱，只关注最重要目标
    //   2. 提升性能（减少绘制数量）
    //
    // 【扩展指南】
    //   要添加新的过滤规则：
    //   1. 在 MenuSwitches.h 添加 bool 过滤字段
    //   2. 在本函数添加 ImGui::Checkbox
    //   3. 在游戏绘制层 Hook 中使用这些过滤条件
    // ================================================================
    static void DrawEspFilterPane()
    {
        ImGui::TextUnformatted("过滤 - Filter");  // 对应 mofang Esp04 面板
        ImGui::Separator();

        // /* 僵尸目标 (Zombie Filter)
        //  * 勾选后 ESP 对僵尸类型敌人生效。
        //  * 僵尸通常移动有规律，适合开启方框追踪。 */
        // ImGui::Checkbox("过滤死亡", &MenuSwitches::g_Toggles.FilterZombie);

        // /* 吸血鬼目标 (Vampire Filter)
        //  * 勾选后 ESP 对吸血鬼类型敌人生效。
        //  * 吸血鬼通常移动更快，骨骼瞄准更推荐。 */
        // ImGui::Checkbox("吸血鬼目标", &MenuSwitches::g_Toggles.FilterVampire);

        // /* 忽略队友 (Ignore Teammate)
        //  * 开启后不显示队友的 ESP 信息。
        //  * 避免屏幕上出现过多无用标记。 */
        // ImGui::Checkbox("忽略队友", &MenuSwitches::g_Toggles.FilterTeammate);

        // /* 仅可见目标 (Visible Only)
        //  * 开启后只渲染玩家视野范围内可见的目标。
        //  * 被墙壁等遮挡的敌人不显示 ESP。 */
        // ImGui::Checkbox("仅可见目标", &MenuSwitches::g_Toggles.FilterVisibleOnly);

        /* 过滤死亡目标 (Filter Dead)
         * 开启后排除已经死亡的目标，只有存活敌人显示 ESP。 */
        ImGui::Checkbox("过滤死亡目标", &MenuSwitches::g_Toggles.FilterDead);
        MenuTooltip("排除已死亡目标，只显示存活敌人");
    }

    // ================================================================
    // 函数：DrawSettingsTab —— 全局设置页面（mofang Setting 页）
    //
    // 【功能说明】
    //   管理所有与菜单本身相关的设置：
    //   - 显示选项（FPS、Tooltips 等）
    //   - UI 参数（透明度、主题、缩放等）
    //   - 性能选项（最大 FPS、垂直同步、性能模式）
    //   - 安全选项（流媒体隐藏、紧急键等）
    //   - 日志管理
    //
    // 【mofang 对照】
    //   对应 mofang/menu.h 中的 Setting 函数。
    //   注意：mofang 的设置页还包含密钥/验证信息，
    //   此处改为通用设置项。
    //
    // 【扩展指南】
    //   添加新设置项：
    //   1. 在 MenuSwitches.h 添加字段
    //   2. 在本函数添加对应控件
    //   3. 在 Menu.cpp 的 Render 函数或相关位置应用设置
    // ================================================================
    void DrawSettingsTab()
    {
        ImGui::TextUnformatted("设置 - Element Setup");  // 对应 mofang Setting 面板
        ImGui::Separator();
        ImGui::Spacing();

        // ---- 显示选项 (Display) ----
        ImGui::SeparatorText("显示 - Display");

        /* 显示工具提示 (Show Tooltips)
         * 鼠标悬停在菜单控件上时显示功能说明文字。
         * 新手建议开启，熟练后可关闭。 */
        ImGui::Checkbox("显示功能提示", &MenuSwitches::g_Toggles.ShowTooltips);
        MenuTooltip("鼠标悬停在菜单控件上时显示功能说明，新手建议开启");

        /* 显示 FPS (Show FPS)
         * 在屏幕角落显示当前帧率。
         * 用于判断性能影响程度。 */
        ImGui::Checkbox("显示 FPS 计数器", &MenuSwitches::g_Toggles.ShowFps);
        MenuTooltip("在屏幕角落显示帧率，判断性能影响程度");
        
        // 折叠子选项：分别控制游戏FPS和覆盖层FPS的显示
        if (MenuSwitches::g_Toggles.ShowFps)
        {
            ImGui::Indent(10.0f);  // 增加缩进量，使层级更明显
            ImGui::Checkbox("显示游戏 FPS", &MenuSwitches::g_Toggles.ShowGameFps);
            MenuTooltip("显示游戏引擎原始帧率");
            ImGui::Checkbox("显示覆盖层 FPS", &MenuSwitches::g_Toggles.ShowOverlayFps);
            MenuTooltip("显示覆盖层渲染帧率");
            ImGui::Unindent(10.0f);
        }

        
        
        ImGui::SeparatorText("调试 - World");

        ImGui::Checkbox("显示世界对象列表", &MenuSwitches::g_Toggles.PreviewWorldObjects);
        MenuTooltip("调试功能：列出场景中所有世界对象");
        ImGui::Checkbox("显示实体信息", &MenuSwitches::g_Toggles.PreviewEntityInfo);
        MenuTooltip("调试功能：显示实体详细信息");
        ImGui::Checkbox("绘制对象指针位置", &MenuSwitches::g_Toggles.DrawObjectPointers);
        MenuTooltip("在屏幕上绘制所有GameObject指针的位置");
        ImGui::Checkbox("绘制敌人位置", &MenuSwitches::g_Toggles.ShowEnemyESP);
        MenuTooltip("在屏幕上绘制 PVE_EnimyList 中敌人的位置（红色方块+序号）");

        // 调试按钮：打印 xxx 的 xxx 地址到控制台
        if (ImGui::Button("打印测试地址"))
        {
            Engine::PrintNeedDataZz();
        }
        MenuTooltip("将 需要测试的 实例及其 偏移后的 指针打印到控制台窗口");

        ImGui::Spacing();
        ImGui::SeparatorText("类实例调试器 - Class Debugger");

        // 总开关：启用后自动循环搜索+绘制
        bool prevEnabled = Engine::ClassDebugger::g_DebugState.enabled;
        ImGui::Checkbox("启用调试器（总开关）", &Engine::ClassDebugger::g_DebugState.enabled);
        MenuTooltip("开启后自动循环搜索类实例并绘制位置，关闭则停止一切");

        // 刚开启时立即触发首次搜索
        if (!prevEnabled && Engine::ClassDebugger::g_DebugState.enabled)
        {
            Engine::ClassDebugger::g_DebugState.lastSearchTime = -999.0f;
            PushLog("[ClassDebugger] 调试器已启动，自动循环搜索+绘制");
        }
        if (prevEnabled && !Engine::ClassDebugger::g_DebugState.enabled)
        {
            PushLog("[ClassDebugger] 调试器已关闭");
        }

        ImGui::SameLine();
        ImGui::Checkbox("显示名称", &Engine::ClassDebugger::g_DebugState.showName);
        MenuTooltip("在屏幕上显示实例的名称");
        ImGui::SameLine();
        ImGui::Checkbox("显示地址", &Engine::ClassDebugger::g_DebugState.showAddress);
        MenuTooltip("在屏幕上显示实例的内存地址");

        // 类名输入框（撑满可用宽度）
        ImGui::PushItemWidth(-1);
        ImGui::InputTextWithHint("类名:", "输入类名 (如 PVE_Player_Enemy)",
            Engine::ClassDebugger::g_DebugState.classNameBuf, 256);
        MenuTooltip("输入要搜索的IL2CPP类名，支持模糊匹配(大小写不敏感)");
        ImGui::PopItemWidth();

        // 手动搜索按钮（立即搜索一次）
        if (ImGui::Button("立即搜索", ImVec2(-1, 0)))
        {
            PushLog("[ClassDebugger] 手动搜索...");
            Engine::ClassDebugger::SearchInstances();
            PushLog(Engine::ClassDebugger::g_DebugState.statusMsg.c_str());
        }

        // 自动搜索间隔
        ImGui::SliderFloat("搜索间隔(秒)", &Engine::ClassDebugger::g_DebugState.autoSearchInterval, 1.0f, 30.0f, "%.1f s");
        MenuTooltip("自动搜索的时间间隔，值越小刷新越快但开销越高");

        // 附加选项
        ImGui::SliderFloat("最大显示距离", &Engine::ClassDebugger::g_DebugState.maxDistance, 50.0f, 2000.0f, "%.0f m");
        MenuTooltip("超出此距离的实例不在屏幕上绘制");
        ImGui::ColorEdit4("文字颜色", Engine::ClassDebugger::g_DebugState.textColor,
            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
        MenuTooltip("调试器在屏幕上绘制文字的颜色");

        // 状态信息显示
        if (!Engine::ClassDebugger::g_DebugState.statusMsg.empty())
        {
            ImGui::TextColored(
                Engine::ClassDebugger::g_DebugState.searchDone
                    ? ImVec4(0.3f, 1.0f, 0.3f, 1.0f)
                    : ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
                "%s", Engine::ClassDebugger::g_DebugState.statusMsg.c_str());
        }

        // 实例列表（实时更新：新增的自动出现，消失的自动移除）
        if (!Engine::ClassDebugger::g_DebugState.instances.empty())
        {
            ImGui::Spacing();
            if (ImGui::BeginChild("IL", "Instances", ImVec2(0, 150), true))
            {
                int idx = 0;
                for (auto& inst : Engine::ClassDebugger::g_DebugState.instances)
                {
                    ImGui::Text("[%d] %s  0x%p  (%.1f, %.1f, %.1f)%s",
                        idx++, inst.name.c_str(), inst.pObject,
                        inst.worldPos.x, inst.worldPos.y, inst.worldPos.z,
                        inst.hasValidPos ? "" : " [无坐标]");
                }
            }
            ImGui::EndChild();
        }

        ImGui::Spacing();

        // ---- UI 参数 (UI) ----
        ImGui::SeparatorText("界面 - UI");

        /* 菜单透明度 (Menu Alpha)
         * 控制整个菜单窗口的不透明度。
         * 值越小越透明，便于在菜单开启时仍看到游戏画面。 */
        ImGui::SliderFloat("菜单透明度", &MenuSwitches::g_Toggles.MenuAlpha, 0.5f, 1.0f, "%.2f");
        MenuTooltip("控制整个菜单窗口的不透明度，值越小越透明");

        /* 主题选择 (Theme Selection)
         * 切换菜单的暗色/亮色主题。
         * Dark = 深色主题（推荐游戏内使用）
         * Light = 浅色主题 */
        ImGui::Combo("主题选择", &MenuSwitches::g_Toggles.ThemeIndex, "深色主题\0浅色主题\0\0");
        MenuTooltip("切换菜单暗色/亮色主题，深色主题推荐游戏内使用");

        /* UI 缩放 (UI Scale)
         * 整体菜单元素的大小缩放比例。
         * 高分辨率屏幕可适当调大(1.2-1.5)。
         * 低分辨率屏幕可调小(0.8-1.0)。 */
        ImGui::SliderFloat("UI缩放比例", &MenuSwitches::g_Toggles.UiScale, 0.8f, 1.5f, "%.2f");
        MenuTooltip("整体菜单缩放比例，高分辨率可调大，低分辨率可调小");

        ImGui::Spacing();

        // ---- 性能选项 (Performance) ----
        ImGui::SeparatorText("性能 - Performance");

        // /* 最大 FPS 限制 (Max FPS)
        //  * 限制菜单/覆盖层的最大帧率。
        //  * 较低的值可以减少 CPU/GPU 占用。
        //  * 0 = 不限制（默认），60-144 适合大多数场景。 */
        // ImGui::SliderFloat("最大 FPS", &MenuSwitches::g_Toggles.MaxFps, 30.0f, 240.0f, "%.0f FPS");

        // /* 垂直同步 (VSync)
        //  * 开启后菜单刷新与显示器刷新率同步。
        //  * 可以减少画面撕裂，但可能增加输入延迟。 */
        // ImGui::Checkbox("垂直同步", &MenuSwitches::g_Toggles.Vsync);

        /* 性能模式 (Performance Mode)
         * 开启后禁用粒子/渐变动画等非必要特效。
         * 建议低配电脑或需要最大 FPS 时开启。 */
        ImGui::Checkbox("性能模式", &MenuSwitches::g_Toggles.PerformanceMode);
        MenuTooltip("关闭粒子/渐变动画等特效以提升帧率，低配电脑建议开启");
        ImGui::SameLine();
        ImGui::TextDisabled("关闭粒子/特效以提升帧率");

        ImGui::Spacing();

        // // ---- 安全与控制 (Security) ----
        // ImGui::SeparatorText("安全 - Security");

        // /* 流媒体隐藏 (Stream Proof)
        //  * 开启后菜单在 OBS/直播软件采集时自动隐藏。
        //  * 适合直播或录屏场景，保护隐私。 */
        // ImGui::Checkbox("流媒体隐藏", &MenuSwitches::g_Toggles.StreamProof);

        // /* 紧急键 (Panic Key)
        //  * 开启后允许使用紧急快捷键（默认 Delete/Home）
        //  * 立即关闭菜单和所有覆盖层。 */
        // ImGui::Checkbox("启用紧急键", &MenuSwitches::g_Toggles.PanicKeyEnabled);

        // ImGui::Spacing();

        // 应用主题色相参数
        if (ImGui::Button("应用主题色相", ImVec2(220, 0)))
        {
            const bool ok = MenuFeatureStubs::ApplyThemeConfig(
                MenuSwitches::g_Toggles.AccentHue,
                MenuSwitches::g_Toggles.GlobalAlpha,
                MenuSwitches::g_Toggles.UiScale);
            PushLog(ok ? "主题色相已应用" : MenuFeatureStubs::NotImplementedMessage());
        }
        MenuTooltip("根据主题色相生成强调色，并应用全局透明度和缩放");

        ImGui::Spacing();
        // ---- 日志管理 (Log) ----
        ImGui::SeparatorText("日志 - Log");

        if (ImGui::Button("导出日志", ImVec2(100, 0)))
        {
            const bool ok = ExportLogsToFile(kExportLogPath);
            if (ok)
            {
                PushLog("日志已导出到 config\\menu.log");
                ShowToast("日志导出成功", ImVec4(0.45f, 0.95f, 0.55f, 1.0f));
            }
            else
            {
                PushLog("日志导出失败");
                ShowToast("日志导出失败", ImVec4(1.00f, 0.45f, 0.45f, 1.0f));
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("清空日志", ImVec2(100, 0)))
        {
            g_menuState.logs.clear();
            PushLog("日志已清空");
            ShowToast("日志已清空", ImVec4(0.45f, 0.85f, 1.00f, 1.0f));
        }
    }

    // ================================================================
    // 函数：DrawConfigTab —— 配置管理页面（mofang 配置/Config 页）
    //
    // 【功能说明】
    //   管理所有配置文件的读写、重置和自动化操作。
    //   - 保存配置：将当前所有设置写入 JSON 文件
    //   - 读取配置：从 JSON 文件恢复所有设置
    //   - 重置配置：恢复出厂默认值
    //   - 自动保存/读取：游戏启动时自动加载，定时自动保存
    //
    // 【配置文件说明】
    //   settings.json      → 主配置文件，手动保存/读取
    //   settings_auto.json → 自动配置（自动读取/保存策略）
    //
    // 【mofang 对照】
    //   对应 mofang/menu.h 中的 Configuration 函数。
    //   mofang 的配置使用 JSON 序列化读写，此处完全兼容。
    //
    // 【扩展指南】
    //   配置文件系统已在 ConfigSchema.h 中定义，新增配置项后
    //   配置文件的保存/加载会自动包含新项，无需修改本函数。
    // ================================================================
    void DrawConfigTab()
    {
        ImGui::TextUnformatted("配置 - Configuration");
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("保存配置", ImVec2(ImGui::GetContentRegionMax().x - ImGui::GetStyle().WindowPadding.x, 30)))
        {
            const bool ok = Config::SaveConfig(kConfigFileName);
            if (ok)
            {
                PushLog("保存成功：config\\settings.json");
                ShowToast("配置保存成功", ImVec4(0.45f, 0.95f, 0.55f, 1.0f));
            }
            else
            {
                PushLog("保存失败：config\\settings.json");
                ShowToast("配置保存失败", ImVec4(1.00f, 0.45f, 0.45f, 1.0f));
            }
        }

        if (ImGui::Button("读取配置", ImVec2(ImGui::GetContentRegionMax().x - ImGui::GetStyle().WindowPadding.x, 30)))
        {
            const bool ok = Config::LoadConfig(kConfigFileName);
            if (ok)
            {
                PushLog("读取成功：config\\settings.json");
                ShowToast("配置读取成功", ImVec4(0.45f, 0.95f, 0.55f, 1.0f));
            }
            else
            {
                PushLog("读取失败：config\\settings.json");
                ShowToast("配置读取失败", ImVec4(1.00f, 0.45f, 0.45f, 1.0f));
            }
        }

        if (ImGui::Button("重置配置为默认值", ImVec2(ImGui::GetContentRegionMax().x - ImGui::GetStyle().WindowPadding.x, 30)))
        {
            MenuSwitches::ResetDefaults();
            const bool saveOk = Config::SaveConfig(kConfigFileName);
            Config::SaveAutoSettings(kAutoConfigFileName, MenuSwitches::g_Toggles.AutoLoadPreview, MenuSwitches::g_Toggles.AutoSavePreview);
            PushLog(saveOk ? "配置已重置并写回默认值" : "配置已重置但写回文件失败");
            ShowToast(saveOk ? "配置已重置为默认值" : "配置重置后写回失败", saveOk ? ImVec4(0.45f, 0.85f, 1.00f, 1.0f) : ImVec4(1.00f, 0.45f, 0.45f, 1.0f));
        }

        ImGui::Spacing();
        ImGui::SeparatorText("自动化策略");

        const bool autoLoadBefore = MenuSwitches::g_Toggles.AutoLoadPreview;
        const bool autoSaveBefore = MenuSwitches::g_Toggles.AutoSavePreview;

        ImGui::Checkbox("启动时自动读取配置", &MenuSwitches::g_Toggles.AutoLoadPreview);
        MenuTooltip("DLL注入后自动加载settings.json，恢复上次设置");
        ImGui::Checkbox("运行时自动保存配置", &MenuSwitches::g_Toggles.AutoSavePreview);
        MenuTooltip("运行期间定时自动保存设置到settings.json");

        if (autoLoadBefore != MenuSwitches::g_Toggles.AutoLoadPreview || autoSaveBefore != MenuSwitches::g_Toggles.AutoSavePreview)
        {
            const bool ok = Config::SaveAutoSettings(kAutoConfigFileName, MenuSwitches::g_Toggles.AutoLoadPreview, MenuSwitches::g_Toggles.AutoSavePreview);
            PushLog(ok ? "自动化设置已保存" : "自动化设置保存失败");
            ShowToast(ok ? "自动化设置已保存" : "自动化设置保存失败", ok ? ImVec4(0.45f, 0.95f, 0.55f, 1.0f) : ImVec4(1.00f, 0.45f, 0.45f, 1.0f));
        }

        ImGui::Spacing();
        if (ImGui::Button("执行配置流程(测试)", ImVec2(ImGui::GetContentRegionMax().x - ImGui::GetStyle().WindowPadding.x, 30)))
        {
            bool ok = true;
            if (MenuSwitches::g_Toggles.AutoLoadPreview)
                ok = Config::LoadConfig(kConfigFileName) && ok;
            if (MenuSwitches::g_Toggles.AutoSavePreview)
                ok = Config::SaveConfig(kConfigFileName) && ok;
            PushLog(ok ? "配置流程执行完成" : "配置流程执行失败");
            ShowToast(ok ? "配置流程执行完成" : "配置流程执行失败", ok ? ImVec4(0.45f, 0.95f, 0.55f, 1.0f) : ImVec4(1.00f, 0.45f, 0.45f, 1.0f));
        }

        ImGui::Spacing();
        ImGui::SeparatorText("文件信息");
        ImGui::TextDisabled("主配置文件: config\\settings.json");
        ImGui::TextDisabled("自动策略文件: config\\settings_auto.json");
    }

    // ================================================================
    // 函数：DrawMemoryTab —— 内存/世界功能页面（mofang 内存/Memory 页）
    //
    // 【功能说明】
    //   控制与游戏内存修改相关的功能：
    //   - 无后坐力 (No Recoil): 消除武器开火后坐力
    //   - 快速射击 (Rapid Fire): 突破武器射速限制
    //   - 无扩散 (No Spread): 消除子弹散布
    //   - 移动加速 (Speed Hack): 提升角色移动速度
    //   - 无限弹药 (Infinite Ammo): 弹药不减
    //
    // 【安全提醒】
    //   本页功能需要直接读写游戏内存地址。
    //   使用前请确认对应地址偏移正确，避免游戏崩溃。
    //   某些反作弊系统会检测此类操作。
    //
    // 【mofang 对照】
    //   对应 mofang/menu.h 中的 Memory 函数。
    //
    // 【扩展指南】
    //   添加新内存功能：
    //   1. 在 MenuSwitches.h 添加字段
    //   2. 在 MenuSwitches.cpp 初始化默认值
    //   3. 在本函数添加 ImGui 控件
    //   4. 在 Engine/Engine.cpp 中实现实际内存读写
    // ================================================================
    static void DrawMemoryTab()
    {
        ImGui::TextUnformatted("内存功能 - Memory");
        ImGui::Separator();
        // ImGui::Spacing();

        // ImGui::SeparatorText("武器修改 - Weapon");

        // ImGui::Checkbox("无后坐力", &MenuSwitches::g_Toggles.PreviewNoRecoil);
        // ImGui::Checkbox("快速射击", &MenuSwitches::g_Toggles.PreviewRapidFire);
        // ImGui::Checkbox("无散布", &MenuSwitches::g_Toggles.PreviewNoSpread);
        // ImGui::Checkbox("无限弹药", &MenuSwitches::g_Toggles.PreviewInfiniteAmmo);

        // ImGui::Spacing();
        // ImGui::SeparatorText("角色修改 - Player"); //

        // ImGui::Checkbox("移动加速", &MenuSwitches::g_Toggles.PreviewSpeedHack);
        // ImGui::SliderFloat("速度倍率", &MenuSwitches::g_Toggles.PreviewSpeedScale, 1.0f, 3.0f, "%.2f 倍");
        // ImGui::SliderInt("弹药值", &MenuSwitches::g_Toggles.PreviewAmmoValue, 30, 9999);



        ImGui::Spacing();
        if (ImGui::Button("提交内存参数(预留接口)", ImVec2(220, 0)))
        {
            const bool ok = MenuFeatureStubs::ApplyMemoryPreview(
                MenuSwitches::g_Toggles.PreviewSpeedScale,
                MenuSwitches::g_Toggles.PreviewAmmoValue);
            PushLog(ok ? "内存参数提交成功" : MenuFeatureStubs::NotImplementedMessage());
        }

        ImGui::TextDisabled("警告：本页功能需对接游戏内存偏移地址，当前为界面展示");
    }

    // ==================== 主题颜色切换 ====================

    // 根据主题索引更新 c:: 命名空间中的颜色常量
    // ThemeIndex: 0=深色主题, 1=浅色主题
    static void ApplyThemeColors(int themeIndex)
    {
        static int lastThemeIndex = -1; // 上次应用的主题索引，-1 表示未初始化
        if (themeIndex == lastThemeIndex)
            return; // 主题未变化，跳过

        lastThemeIndex = themeIndex;

        if (themeIndex == 1)
        {
            // ---- 浅色主题 ----
            c::accent_text_color       = ImColor(20, 20, 30, 255);
            c::accent_text_low_color   = ImColor(20, 20, 30, 255 / 2);
            c::text::text_hov          = ImColor(20, 20, 30, 255);
            c::text::text              = ImColor(120, 123, 130, 255);
            c::text::hide_text         = ImColor(210, 212, 216, 255);

            c::bg::background          = ImColor(240, 240, 245, 255);
            c::bg::outline_background  = ImColor(200, 200, 210, 255);

            c::child::background          = ImColor(230, 230, 235, 250);
            c::child::outline_background  = ImColor(200, 200, 210, 255);

            c::checkbox::circle_inactive  = ImColor(210, 212, 216, 255);
            c::checkbox::background       = ImColor(225, 225, 230, 255);
            c::checkbox::outline_background = ImColor(200, 200, 210, 255);

            c::slider::circle_inactive    = ImColor(210, 212, 216, 255);
            c::slider::background         = ImColor(225, 225, 230, 255);
            c::slider::outline_background = ImColor(200, 200, 210, 255);

            c::combo::background          = ImColor(225, 225, 230, 255);
            c::combo::outline_background  = ImColor(200, 200, 210, 255);

            c::picker::background         = ImColor(225, 225, 230, 255);
            c::picker::outline_background = ImColor(200, 200, 210, 255);

            c::button::background         = ImColor(225, 225, 230, 255);
            c::button::outline_background = ImColor(200, 200, 210, 255);

            c::input::background          = ImColor(225, 225, 230, 255);
            c::input::outline_background  = ImColor(200, 200, 210, 255);

            c::keybind::background        = ImColor(225, 225, 230, 255);
            c::keybind::outline_background = ImColor(200, 200, 210, 255);

            c::notify = ImColor(225, 225, 230, 255);
        }
        else
        {
            // ---- 深色主题（默认） ----
            c::accent_text_color       = ImColor(245, 245, 255);
            c::accent_text_low_color   = ImColor(245, 245, 255, 255 / 2);
            c::text::text_hov          = ImColor(245, 245, 255);
            c::text::text              = ImColor(90, 93, 100, 255);
            c::text::hide_text         = ImColor(43, 48, 54, 255);

            c::bg::background          = ImColor(14, 14, 15, 255);
            c::bg::outline_background  = ImColor(27, 29, 32, 255);

            c::child::background          = ImColor(22, 23, 26, 250);
            c::child::outline_background  = ImColor(27, 29, 32, 255);

            c::checkbox::circle_inactive  = ImColor(43, 48, 54, 255);
            c::checkbox::background       = ImColor(27, 29, 32, 255);
            c::checkbox::outline_background = ImColor(30, 32, 36, 255);

            c::slider::circle_inactive    = ImColor(43, 48, 54, 255);
            c::slider::background         = ImColor(27, 29, 32, 255);
            c::slider::outline_background = ImColor(30, 32, 36, 255);

            c::combo::background          = ImColor(27, 29, 32, 255);
            c::combo::outline_background  = ImColor(30, 32, 36, 255);

            c::picker::background         = ImColor(27, 29, 32, 255);
            c::picker::outline_background = ImColor(30, 32, 36, 255);

            c::button::background         = ImColor(27, 29, 32, 255);
            c::button::outline_background = ImColor(30, 32, 36, 255);

            c::input::background          = ImColor(27, 29, 32, 255);
            c::input::outline_background  = ImColor(30, 32, 36, 255);

            c::keybind::background        = ImColor(27, 29, 32, 255);
            c::keybind::outline_background = ImColor(30, 32, 36, 255);

            c::notify = ImColor(43, 48, 54);
        }
    }

    // ==================== 主渲染函数 ====================

    void Render()
    {
        if (!IsMenuOpen)
            return;

        // ---- 应用主题颜色 ----
        ApplyThemeColors(MenuSwitches::g_Toggles.ThemeIndex);

        // ---- 应用 UI 缩放到全局字体 ----
        // 仅在值实际变化时更新 FontGlobalScale，避免快速滑动时 ImGui 内部布局索引不一致
        {
            static float lastFontScale = 1.0f;
            const float newScale = MenuSwitches::g_Toggles.UiScale;
            if (newScale != lastFontScale)
            {
                ImGui::GetIO().FontGlobalScale = newScale;
                lastFontScale = newScale;
            }
        }

        static float tab_size = 0.f;
        static float arrow_roll = 0.f;
        static bool tab_opening = true;

        // 左侧 tab 区展开/收起动画（受 UiScale 影响）。
        const float uiScale = ImClamp(MenuSwitches::g_Toggles.UiScale, 0.8f, 1.5f);
        tab_size = ImLerp(tab_size, tab_opening ? 160.f * uiScale : 0.f, ImGui::GetIO().DeltaTime * 12.f);
        arrow_roll = ImLerp(arrow_roll, tab_opening && (tab_size >= 159.f * uiScale) ? 1.f : -1.f, ImGui::GetIO().DeltaTime * 12.f);

        // 使用 PushStyleVar 而非直接修改全局 style，防止影响滑条等控件自身渲染
        ImGuiStyle& s = ImGui::GetStyle();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(20 * uiScale, 20 * uiScale));
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 8.f * uiScale);

        if (image::bg)
        {
            if (MenuSwitches::g_Toggles.PerformanceMode)
            {
                // 性能模式下避免全屏贴图采样，改为纯色遮罩。
                ImGui::GetBackgroundDrawList()->AddRectFilled(
                    ImVec2(0, 0),
                    ImGui::GetIO().DisplaySize,
                    IM_COL32(10, 12, 16, 220));
            }
            else
            {
                ImGui::GetBackgroundDrawList()->AddImage(image::bg, ImVec2(0, 0), ImVec2(1920, 1080), ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255));
            }
        }
        else
        {
            // 无贴图时的降级背景，保证菜单始终可读。
            ImDrawList* bg = ImGui::GetBackgroundDrawList();
            const ImVec2 display = ImGui::GetIO().DisplaySize;
            bg->AddRectFilledMultiColor(
                ImVec2(0, 0),
                display,
                IM_COL32(8, 10, 14, 210),
                IM_COL32(8, 10, 14, 210),
                IM_COL32(16, 18, 24, 220),
                IM_COL32(16, 18, 24, 220));

            if (!MenuSwitches::g_Toggles.PerformanceMode)
            {
                bg->AddCircleFilled(ImVec2(display.x * 0.15f, display.y * 0.18f), 220.0f, IM_COL32(250, 132, 86, 22), 96);
                bg->AddCircleFilled(ImVec2(display.x * 0.82f, display.y * 0.72f), 260.0f, IM_COL32(87, 188, 255, 18), 96);
            }
        }

        // 应用缩放后的菜单窗口尺寸
        const ImVec2 scaledBgSize = ImVec2(c::bg::size.x * uiScale, c::bg::size.y * uiScale);
        ImGui::SetNextWindowSize(scaledBgSize + ImVec2(tab_size, 0));

        // 应用菜单透明度
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, MenuSwitches::g_Toggles.MenuAlpha);

        ImGui::Begin("IMGUI MENU", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus);
        {
            const ImVec2& pos = ImGui::GetWindowPos();
            const ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
           
            ImGui::GetBackgroundDrawList()->AddRectFilled(pos, pos + scaledBgSize + ImVec2(tab_size, 0), ImGui::GetColorU32(c::bg::background), c::bg::rounding * uiScale);
            ImGui::GetBackgroundDrawList()->AddRect(pos, pos + scaledBgSize + ImVec2(tab_size, 0), ImGui::GetColorU32(c::bg::outline_background), c::bg::rounding * uiScale);

            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(c::accent_text_color));

            if (font::pTahomaBold2) ImGui::PushFont(font::pTahomaBold2);
            ImGui::RenderTextClipped(pos + ImVec2(60, 0) + spacing, pos + spacing + ImVec2(60, 60) + ImVec2(tab_size + (spacing.x / 2) - 30, 0), "AndThen", NULL, NULL, ImVec2(0.5f, 0.5f), NULL); 
            if (font::pTahomaBold2) ImGui::PopFont();

            ImGui::RenderTextClipped(pos + ImVec2(60 * uiScale + spacing.x, scaledBgSize.y - 60 * 2 * uiScale), pos + spacing + ImVec2(60 * uiScale, scaledBgSize.y) + ImVec2(tab_size, 0), "剩余时间_2099-9-9", NULL, NULL, ImVec2(0.0f, 0.43f), NULL);
            ImGui::RenderTextClipped(pos + ImVec2(60 * uiScale + spacing.x, scaledBgSize.y - 60 * 2 * uiScale), pos + spacing + ImVec2(60 * uiScale, scaledBgSize.y) + ImVec2(tab_size, 0), "然后呢\n", NULL, NULL, ImVec2(0.0f, 0.57f), NULL);

            if (font::pTahomaBold2) ImGui::PushFont(font::pTahomaBold2); 
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(250, 255, 255, 255));
            ImGui::RenderTextClipped(pos + ImVec2(0, 0) + spacing, pos + spacing + ImVec2(60, 40) + ImVec2(tab_size + (spacing.x / 2) + 199, 0), "Hello", NULL, NULL, ImVec2(1.f, 0.5f), NULL); 
            ImGui::PopStyleColor();
            if (font::pTahomaBold2) ImGui::PopFont(); 

            // 暂时注释掉顶部logo，调试重复绘制问题
            /*
            if (image::logo)
            {
                // 使用 GetWindowDrawList() 而不是 GetBackgroundDrawList()，确保 logo 绘制在窗口内部
                ImGui::GetWindowDrawList()->AddImage(image::logo, pos + ImVec2(14, 14), pos + ImVec2(46, 46), ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255));
            }
            else
            {
                // Logo 加载失败时的降级方案：绘制一个渐变圆形（位置与实际 logo 图像一致）
                // logo 图像范围是 (14,14) 到 (46,46)，中心点是 (30,30)，半径是 16
                ImVec2 logoCenter = pos + ImVec2(30, 30);
                ImGui::GetWindowDrawList()->AddCircleFilled(logoCenter, 16.0f, ImColor(71, 226, 67, 255), 32);
                ImGui::GetWindowDrawList()->AddCircle(logoCenter, 16.0f, ImColor(255, 255, 255, 200), 32, 2.0f);
            }
            */

            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(90, 93, 100, 255)); 
            ImGui::RenderTextClipped(pos + ImVec2(30, 20) + spacing, pos + spacing + ImVec2(60, 80) + ImVec2(tab_size + (spacing.x / 2) + 108, -20), "欢迎回来!", NULL, NULL, ImVec2(1.f, 0.5f), NULL); 
            ImGui::PopStyleColor();

            // ===== Tab 页索引（需要在搜索框之前声明，因为搜索结果会修改它）=====
            static int tabs = 0;

            static char Search[256] = { "" };  // 增加缓冲区大小以支持更多中文字符
            static std::vector<MenuSearch::SearchResult> searchResults;
            static bool showSearchResults = false;
            
            // 顶部搜索框
            ImGui::SetCursorPos(ImVec2(385 + tab_size, -20) + (s.ItemSpacing * 2));
            ImGui::BeginChild("", " ", ImVec2(ImMax(1.0f, (scaledBgSize.x - 60 * uiScale - s.ItemSpacing.x * 4) / 2), ImMax(1.0f, 60 * uiScale)));
            if (font::pTabIcon) ImGui::PushFont(font::pTabIcon);
            ImGui::Text("I"); ImGui::SameLine(); ImGui::Text("H"); ImGui::SameLine(); ImGui::Text("G");
            if (font::pTabIcon) ImGui::PopFont(); 
            ImGui::SameLine();
            ImGui::SetNextItemWidth(180);
            
            // 搜索框输入（确保支持中文输入）
            if (ImGui::InputTextWithHint("搜索", "搜索功能...", Search, 256, ImGuiInputTextFlags_None))
            {
                // 输入变化时执行搜索
                if (Search[0] != '\0')
                {
                    MenuSearch::Search(Search, searchResults, 8);  // 最多显示 8 条结果
                    showSearchResults = searchResults.size() > 0;
                }
                else
                {
                    showSearchResults = false;
                }
            }
            
            // 检测搜索框是否获得焦点
            const bool searchBoxActive = ImGui::IsItemActive();
            
            ImGui::EndChild();
            
            // ===== 搜索结果下拉框 =====
            if (showSearchResults)
            {
                ImGui::SetNextWindowPos(ImVec2(pos.x + 385 + tab_size + s.ItemSpacing.x * 2, pos.y + 60 + s.ItemSpacing.y));
                ImGui::SetNextWindowSize(ImVec2(320, 0));  // 宽度固定，高度自适应
                ImGui::SetNextWindowBgAlpha(0.95f);
                
                // 添加 NoFocusOnAppearing 和 NoNavFocus 标志，防止窗口抢夺输入框焦点
                ImGuiWindowFlags searchWindowFlags = 
                    ImGuiWindowFlags_NoDecoration | 
                    ImGuiWindowFlags_NoMove | 
                    ImGuiWindowFlags_AlwaysAutoResize |
                    ImGuiWindowFlags_NoSavedSettings |
                    ImGuiWindowFlags_NoFocusOnAppearing |  // 防止出现时抢夺焦点
                    ImGuiWindowFlags_NoNavFocus;           // 防止获得导航焦点
                
                if (ImGui::Begin("##SearchResults", nullptr, searchWindowFlags))
                {
                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "搜索结果 (%d)", static_cast<int>(searchResults.size()));
                    ImGui::Separator();
                    
                    for (size_t i = 0; i < searchResults.size(); ++i)
                    {
                        const MenuSearch::SearchResult& result = searchResults[i];
                        const MenuSearch::SearchableItem* item = result.item;
                        
                        ImGui::PushID(static_cast<int>(i));
                        
                        // 功能名称（使用默认中文字体以正确显示中文）
                        const bool clicked = ImGui::Selectable(item->displayName, false, 0, ImVec2(300, 0));
                        
                        // 描述和所属页面（小字）
                        ImGui::Indent(10.0f);
                        ImGui::TextDisabled("%s | %s", item->description, MenuSearch::GetTabName(item->tabIndex));
                        ImGui::Unindent(10.0f);
                        
                        // 点击后跳转到对应 Tab
                        if (clicked)
                        {
                            tabs = item->tabIndex;
                            // 清空搜索框和结果
                            Search[0] = '\0';
                            showSearchResults = false;
                        }
                        
                        ImGui::PopID();
                    }
                    
                    // 检测是否点击了搜索结果窗口外部
                    if (ImGui::IsMouseClicked(0) && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
                    {
                        showSearchResults = false;
                    }
                }
                ImGui::End();
            }
            
            ImGui::PopStyleColor(1);

            const char* nametab_array1[6] = { "E", "D", "A", "G", "C", "B" };
            const char* nametab_array[6] = { "瞄准", "视觉", "颜色", "内存", "设置", "配置" };
            const char* nametab_hint_array[6] = { "Aim, Recoil, Trigger", "Overlay, Chams, World", "Game Colors", "World Memory", "Element Setup", "Save Settings" };

            ImGui::SetCursorPos(ImVec2(spacing.x + (60 * uiScale - 22) / 2, 60 * uiScale + (spacing.y * 2) + 22));
            ImGui::BeginGroup();
            {
                for (int i = 0; i < 6; i++)
                    if (ImGui::Tab(i == tabs, nametab_array1[i], nametab_array[i], nametab_hint_array[i], ImVec2(35 + tab_size, 35))) tabs = i;
            }
            ImGui::EndGroup();

            ImGui::SetCursorPos(ImVec2(8, 9) + spacing);

            // 侧边栏收起按钮：
            // 1) 有贴图时用原始图标按钮
            // 2) 无贴图时用文本箭头兜底，确保用户始终看得到入口
            if (image::roll)
            {
                ImRotateStart();
                if (ImGui::CustomButton(1, image::roll, ImVec2(35, 35), ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(c::accent_color)))
                    tab_opening = !tab_opening;
                ImRotateEnd(1.57f * arrow_roll);
            }
            else
            {
                const char* collapseIcon = tab_opening ? "<" : ">";
                if (ImGui::Button(collapseIcon, ImVec2(35, 35)))
                    tab_opening = !tab_opening;
            }

            ImGui::GetBackgroundDrawList()->AddRectFilled(pos + ImVec2(0, -20 * uiScale + spacing.y) + spacing, pos + spacing + ImVec2(60 * uiScale, scaledBgSize.y - 60 * uiScale - spacing.y * 3) + ImVec2(tab_size, 0), ImGui::GetColorU32(c::child::background), c::child::rounding * uiScale);
            ImGui::GetBackgroundDrawList()->AddRect(pos + ImVec2(0, -20 * uiScale + spacing.y) + spacing, pos + spacing + ImVec2(60 * uiScale, scaledBgSize.y - 60 * uiScale - spacing.y * 3) + ImVec2(tab_size, 0), ImGui::GetColorU32(c::child::outline_background), c::child::rounding * uiScale);

            ImGui::GetBackgroundDrawList()->AddRectFilled(pos + ImVec2(0, scaledBgSize.y - 60 * uiScale - spacing.y * 2) + spacing, pos + spacing + ImVec2(60 * uiScale, scaledBgSize.y - spacing.y * 2) + ImVec2(tab_size, 0), ImGui::GetColorU32(c::child::background), c::child::rounding * uiScale);
            ImGui::GetBackgroundDrawList()->AddRect(pos + ImVec2(0, scaledBgSize.y - 60 * uiScale - spacing.y * 2) + spacing, pos + spacing + ImVec2(60 * uiScale, scaledBgSize.y - spacing.y * 2) + ImVec2(tab_size, 0), ImGui::GetColorU32(c::child::outline_background), c::child::rounding * uiScale);

            if (image::logo)
            {
                ImGui::GetWindowDrawList()->AddImage(image::logo, pos + ImVec2(0, scaledBgSize.y - 60 * uiScale - spacing.y * 2) + spacing + ImVec2(12, 12), pos + spacing + ImVec2(60 * uiScale, scaledBgSize.y - spacing.y * 2) - ImVec2(12, 12), ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255));
            }
            else
            {
                // Logo 加载失败时的降级方案：绘制一个渐变圆形（位置与实际 logo 图像一致）
                ImVec2 logoCenter = pos + ImVec2(0, scaledBgSize.y - 60 * uiScale - spacing.y * 2) + spacing + ImVec2(30, 30);
                ImGui::GetWindowDrawList()->AddCircleFilled(logoCenter, 18.0f, ImColor(71, 226, 67, 255), 32);
                ImGui::GetWindowDrawList()->AddCircle(logoCenter, 18.0f, ImColor(255, 255, 255, 200), 32, 2.0f);
            }

            ImGui::GetWindowDrawList()->AddCircleFilled(pos + ImVec2(63 * uiScale, scaledBgSize.y - (spacing.y * 2) + 3), 10.f, ImGui::GetColorU32(c::child::background), 100);
            ImGui::GetWindowDrawList()->AddCircleFilled(pos + ImVec2(63 * uiScale, scaledBgSize.y - (spacing.y * 2) + 3), 6.f, ImColor(0, 255, 0, 255), 100);

            Particles();

            static float tab_alpha = 0.f; static float tab_add = 0.f; static int active_tab = 0;

            tab_alpha = ImClamp(tab_alpha + (4.f * ImGui::GetIO().DeltaTime * (tabs == active_tab ? 1.f : -1.f)), 0.f, 1.f);
            tab_add = ImClamp(tab_add + (std::round(350.f) * ImGui::GetIO().DeltaTime * (tabs == active_tab ? 1.f : -1.f)), 0.f, 1.f);

            if (tab_alpha == 0.f && tab_add == 0.f) active_tab = tabs;

            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, tab_alpha * s.Alpha);

            if (tabs == 0)
            {
                // Aim 页布局：主区域 + 底部两个次区域。
                ImGui::SetCursorPos(ImVec2(60 * uiScale + tab_size, 60 * uiScale) + (s.ItemSpacing * 2));
                ImGui::BeginGroup();
                {
                    ImGui::BeginChild("D", "Main", ImVec2(ImMax(1.0f, (scaledBgSize.x - 60 * uiScale - s.ItemSpacing.x * 4) + 18 * uiScale), ImMax(1.0f, 405 * uiScale)));
                    {
                        DrawAimbotTab();
                    }
                    ImGui::EndChild();
                }
                ImGui::EndGroup();
                
                ImGui::SameLine();
                ImGui::BeginGroup();
                {
                    ImGui::SetCursorPos(ImVec2(60 * uiScale + tab_size, 480 * uiScale) + (s.ItemSpacing * 2));
                    ImGui::BeginChild("A", "Other", ImVec2(ImMax(1.0f, (scaledBgSize.x - 60 * uiScale - s.ItemSpacing.x * 4) / 2), ImMax(1.0f, 100 * uiScale)));
                    {
                        // Aim_bot02 data
                    }
                    ImGui::EndChild();

                    ImGui::SetCursorPos(ImVec2(380 * uiScale + tab_size, 480 * uiScale) + (s.ItemSpacing * 2));
                    ImGui::BeginChild("E", "Setting", ImVec2(ImMax(1.0f, (scaledBgSize.x - 60 * uiScale - s.ItemSpacing.x * 4) / 2), ImMax(1.0f, 100 * uiScale)));
                    {
                        // Aim_bot03 data
                    }
                    ImGui::EndChild();
                }
                ImGui::EndGroup();
            }
            if (tabs == 1)
            {
                // 视觉页布局：左主右辅 + 底部筛选块。
                ImGui::SetCursorPos(ImVec2(60 * uiScale + tab_size, 60 * uiScale) + (s.ItemSpacing * 2));
                ImGui::BeginGroup();
                {
                    ImGui::BeginChild("D", "Main", ImVec2(ImMax(1.0f, (scaledBgSize.x - 60 * uiScale - s.ItemSpacing.x * 4) / 2), ImMax(1.0f, 420 * uiScale)));
                    {
                        DrawESPTab();
                    }
                    ImGui::EndChild();
                }
                ImGui::EndGroup();
                
                ImGui::SameLine();
                ImGui::BeginGroup();
                {
                    ImGui::BeginChild("E", "Show", ImVec2(ImMax(1.0f, (scaledBgSize.x - 60 * uiScale - s.ItemSpacing.x * 4) / 2), ImMax(1.0f, 420 * uiScale)));
                    {
                        DrawEspShowPane();
                    }
                    ImGui::EndChild();

                    ImGui::BeginChild("B", "Other", ImVec2(ImMax(1.0f, (scaledBgSize.x - 60 * uiScale - s.ItemSpacing.x * 4) / 2), ImMax(1.0f, 80 * uiScale)));
                    {
                        DrawEspOtherPane();
                    }
                    ImGui::EndChild();

                    ImGui::SetCursorPos(ImVec2(60 * uiScale + tab_size, 500 * uiScale) + (s.ItemSpacing * 2));
                    ImGui::BeginChild("A", "Filter ", ImVec2(ImMax(1.0f, (scaledBgSize.x - 60 * uiScale - s.ItemSpacing.x * 4) / 2), ImMax(1.0f, 80 * uiScale)));
                    {
                        DrawEspFilterPane();
                    }
                    ImGui::EndChild();
                }
                ImGui::EndGroup();
            }
            if (tabs == 2)
            {
                // 颜色页：单主面板。
                ImGui::SetCursorPos(ImVec2(60 * uiScale + tab_size, 60 * uiScale) + (s.ItemSpacing * 2));
                ImGui::BeginGroup();
                {
                    ImGui::BeginChild("D", "Main", ImVec2(ImMax(1.0f, (scaledBgSize.x - 60 * uiScale - s.ItemSpacing.x * 4) + 18 * uiScale), ImMax(1.0f, 445 * uiScale)));
                    {
                        DrawOverviewTab();
                    }
                    ImGui::EndChild();
                }
                ImGui::EndGroup();
            }
            if (tabs == 3)
            {
                // 内存页：当前占位，保留版式与高度。
                ImGui::SetCursorPos(ImVec2(60 * uiScale + tab_size, 60 * uiScale) + (s.ItemSpacing * 2));
                ImGui::BeginGroup();
                {
                    ImGui::BeginChild("D", "Main", ImVec2(ImMax(1.0f, (scaledBgSize.x - 60 * uiScale - s.ItemSpacing.x * 4) + 18 * uiScale), ImMax(1.0f, 470 * uiScale)));
                    {
                        DrawMemoryTab();
                    }
                    ImGui::EndChild();
                }
                ImGui::EndGroup();
            }
            if (tabs == 4)
            {
                // 设置页：单主面板。
                ImGui::SetCursorPos(ImVec2(60 * uiScale + tab_size, 60 * uiScale) + (s.ItemSpacing * 2));
                ImGui::BeginGroup();
                {
                    ImGui::BeginChild("D", "Main", ImVec2(ImMax(1.0f, (scaledBgSize.x - 60 * uiScale - s.ItemSpacing.x * 4) + 18 * uiScale), ImMax(1.0f, 495 * uiScale)));
                    {
                        DrawSettingsTab();
                    }
                    ImGui::EndChild();
                }
                ImGui::EndGroup();
            }
            if (tabs == 5)
            {
                // 配置页：单主面板。
                ImGui::SetCursorPos(ImVec2(60 * uiScale + tab_size, 60 * uiScale) + (s.ItemSpacing * 2));
                ImGui::BeginGroup();
                {
                    ImGui::BeginChild("D", "Main", ImVec2(ImMax(1.0f, (scaledBgSize.x - 60 * uiScale - s.ItemSpacing.x * 4) + 18 * uiScale), ImMax(1.0f, 520 * uiScale)));
                    {
                        DrawConfigTab();
                    }
                    ImGui::EndChild();
                }
                ImGui::EndGroup();
            }

            ImGui::PopStyleVar(); // Pop tab_alpha
        }
        ImGui::End();
        ImGui::PopStyleVar(); // Pop MenuAlpha

        // Pop 4 个 PushStyleVar (WindowPadding, WindowBorderSize, ItemSpacing, ScrollbarSize)
        ImGui::PopStyleVar(4);

        DrawToast();
    }

    void Initialize()
    {
        printf("[+] Menu system initialized\n");
    }

    void Shutdown()
    {
        printf("[+] Menu system shutdown\n");
    }

    void ToggleMenu()
    {
        IsMenuOpen = !IsMenuOpen;
    }

} // namespace Menu
