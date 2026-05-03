#include "Menu.h"
#include "Draw.h"
#include "MenuSwitches.h"
#include "MenuFeatureStubs.h"
#include "Config.h"
#include "imgui/imgui_internal.h"
#include <cfloat>
#include <cmath>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <d3d11.h>

// Import ImGui custom settings
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
        ImGui::ColorEdit4("3D方框颜色", MenuSwitches::g_Toggles.ColorBox3D, picker_flags);
        ImGui::ColorEdit4("名字颜色", MenuSwitches::g_Toggles.ColorName, picker_flags);
        ImGui::ColorEdit4("射线颜色", MenuSwitches::g_Toggles.ColorSnapline, picker_flags);
        ImGui::ColorEdit4("距离颜色", MenuSwitches::g_Toggles.ColorDistance, picker_flags);
        ImGui::ColorEdit4("血量颜色", MenuSwitches::g_Toggles.ColorHealth, picker_flags);
        ImGui::ColorEdit4("可见骨骼颜色", MenuSwitches::g_Toggles.ColorBoneVisible, picker_flags);
        ImGui::ColorEdit4("遮挡骨骼颜色", MenuSwitches::g_Toggles.ColorBoneOccluded, picker_flags);
        ImGui::ColorEdit4("雷达颜色", MenuSwitches::g_Toggles.ColorRadar, picker_flags);
        ImGui::ColorEdit4("物品高亮颜色", MenuSwitches::g_Toggles.ColorLoot, picker_flags);

        ImGui::Spacing();
        ImGui::SeparatorText("全局设置");

        ImGui::SliderFloat("主题色相", &MenuSwitches::g_Toggles.AccentHue, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("全局透明度", &MenuSwitches::g_Toggles.GlobalAlpha, 0.60f, 1.0f, "%.2f");
        ImGui::Checkbox("动态渐变", &MenuSwitches::g_Toggles.DynamicGradient);

        ImGui::Spacing();
        if (ImGui::Button("应用主题参数(预留接口)", ImVec2(220, 0)))
        {
            const bool ok = MenuFeatureStubs::ApplyThemeConfig(
                MenuSwitches::g_Toggles.AccentHue,
                MenuSwitches::g_Toggles.GlobalAlpha,
                MenuSwitches::g_Toggles.UiScale);
            PushLog(ok ? "主题参数应用成功" : MenuFeatureStubs::NotImplementedMessage());
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
        ImGui::Checkbox("启用自瞄", &MenuSwitches::g_Toggles.OpenAimbot);

        /* 骨骼瞄准 (Bone Aim)
         * 开启后自瞄会瞄准敌人骨骼位置（如头部/胸部骨骼），
         * 关闭则瞄准敌人中心位置。骨骼瞄准通常更精确。 */
        ImGui::Checkbox("骨骼瞄准", &MenuSwitches::g_Toggles.BoneOn);

        /* 静默自瞄模式 (Silent Aimbot)
         * 开启后客户端不显示准星移动到敌人身上，但子弹会命中。
         * 注意：部分反作弊会检测此类行为。 */
        ImGui::Checkbox("静默自瞄模式", &MenuSwitches::g_Toggles.MemoryAimbot);

        /* 穿透自瞄模式 (Wallbang Aimbot)
         * 开启后即使敌人在墙后也会被瞄准（需配合 Wallbang 功能）。
         * 适合穿墙射击场景。 */
        ImGui::Checkbox("穿透自瞄模式", &MenuSwitches::g_Toggles.Missed_shot);

        ImGui::Spacing();

        // ---- 核心参数滑条 ----

        /* 自瞄最大范围 (Aim Range / Max Distance)
         * 单位：米。只对在此距离内的敌人触发自瞄。
         * 建议：50-100 适合近战，200-500 适合远程狙杀。 */
        ImGui::SliderFloat("自瞄范围(米)", &MenuSwitches::g_Toggles.Aim_Range, 1.f, 500.f, "%.1f");

        /* XY 偏移阈值 (XY Threshold)
         * 当准星与敌人中心的偏移小于此值时触发自瞄修正。
         * 值越小自瞄越"不灵敏"，避免轻微晃动就锁人。 */
        ImGui::SliderFloat("XY偏移阈值", &MenuSwitches::g_Toggles.threshold, 0.f, 30.f, "%.1f");

        ImGui::Spacing();
        ImGui::SeparatorText("高级设置 - Advanced");  // 对应 mofang Aim_bot03 面板

        /* 扳机开火 (Triggerbot)
         * 开启后当准星移动到敌人身上时将自动开枪。
         * 通常结合自瞄或 FOV 限制一起使用。 */
        ImGui::Checkbox("扳机开火", &MenuSwitches::g_Toggles.TriggerBot);

        /* 预测瞄准 (Aim Prediction)
         * 开启后自瞄会考虑敌人的移动速度和方向，
         * 提前预判敌人位置，适合射击移动目标。 */
        ImGui::Checkbox("预测瞄准", &MenuSwitches::g_Toggles.AimPrediction);

        /* FOV 限制开关 (FOV Limit)
         * 开启后只有敌人在指定角度范围内才触发自瞄。
         * 有助于减少视野边缘的误锁。 */
        ImGui::Checkbox("FOV限制", &MenuSwitches::g_Toggles.AimFovLimit);

        /* FOV 角度值 (FOV Angle)
         * 自瞄的有效角度范围（度数）。
         * 以屏幕中心为原点，建议范围 5°-180°。
         * 5°-15°: 极精准/低显性，30°-90°: 中等，180°: 全屏锁 */
        ImGui::SliderFloat("FOV角度", &MenuSwitches::g_Toggles.AimFov, 1.0f, 180.0f, "%.1f°");

        /* 平滑系数 (Aim Smoothness)
         * 控制自瞄从当前位置移动到目标的速度。
         * 1 = 瞬间锁定（暴力锁），10+ = 更自然的人类移动感。
         * 过高可能导致追不上快速移动的敌人。 */
        ImGui::SliderFloat("平滑系数", &MenuSwitches::g_Toggles.AimSmooth, 1.0f, 20.0f, "%.1f");

        /* 骨骼瞄准索引 (Bone Index)
         * 选择自瞄锁定的目标骨骼：
         * 0 = 头部 (Head), 1 = 颈部 (Neck), 2 = 胸部 (Chest) */
        ImGui::SliderInt("骨骼索引(0=头,1=颈,2=胸)", &MenuSwitches::g_Toggles.AimBoneIndex, 0, 2);

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

        /* 名字 ESP (Name ESP)
         * 在方框上方显示目标的名称或标签。
         * 颜色在"颜色"页中配置。 */
        ImGui::Checkbox("名字 ESP", &MenuSwitches::g_Toggles.EspName);

        /* 血量 ESP (Health ESP)
         * 在方框左侧显示血量条 + 血量百分比文字。
         * 绿色 = 高血量，黄色 = 中血量，红色 = 低血量。 */
        ImGui::Checkbox("血量 ESP", &MenuSwitches::g_Toggles.EspHealth);

        /* 距离 ESP (Distance ESP)
         * 在方框下方显示与目标的距离（米）。
         * 可以快速判断哪些敌人威胁最大。 */
        ImGui::Checkbox("距离 ESP", &MenuSwitches::g_Toggles.EspDistance);

        /* 骨骼绘制 (Skeleton)
         * 绘制目标的内部骨骼连接线。
         * 绿色 = 可见骨骼，紫色 = 被遮挡的骨骼（穿墙透视）。 */
        ImGui::Checkbox("骨骼绘制", &MenuSwitches::g_Toggles.EspSkeleton);

        /* 射线/拉线 (Snapline)
         * 从屏幕底部/侧边缘到目标画一条辅助追踪线。
         * 颜色在"颜色"页配置。 */
        ImGui::Checkbox("射线绘制", &MenuSwitches::g_Toggles.EspSnapline);

        ImGui::Spacing();
        ImGui::SeparatorText("扩展功能");

        /* 物资显示 (Loot ESP)
         * 在场景中标记可拾取物品（武器、弹药、护甲等）。
         * 物品颜色在"颜色"页配置。 */
        ImGui::Checkbox("物资显示", &MenuSwitches::g_Toggles.EspLoot);

        /* 雷达显示 (Radar)
         * 在屏幕角落或自定义位置显示小地图雷达，
         * 标记敌人位置和方向。支持圆形/方形两种样式。 */
        ImGui::Checkbox("雷达显示", &MenuSwitches::g_Toggles.EspRadar);

        ImGui::Spacing();

        // ---- ESP 参数滑条 ----

        /* 最大显示距离 (Max Render Distance)
         * 单位为米。超出此距离的目标不会渲染任何 ESP 信息。
         * 提高性能利用率，建议结合游戏实际情况设置。 */
        ImGui::SliderFloat("最大显示距离", &MenuSwitches::g_Toggles.EspMaxDistance, 30.0f, 500.0f, "%.0f米");

        /* 方框线条粗细 (ESP Box Thickness)
         * 控制方框边框的像素宽度，1-3 像素视觉效果较好。 */
        ImGui::SliderFloat("方框线条粗细", &MenuSwitches::g_Toggles.EspThickness, 0.5f, 5.0f, "%.1fpx");

        /* 文字缩放 (Text Scale)
         * 控制所有 ESP 文字的缩放比例，包括名字、距离、血量百分比。
         * 1.0 = 正常大小，需要更大字可调至 1.5-1.8 */
        ImGui::SliderFloat("文字缩放", &MenuSwitches::g_Toggles.EspTextScale, 0.6f, 1.8f, "%.2f");

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
        const bool anyEnabled =
            MenuSwitches::g_Toggles.EspBox ||
            MenuSwitches::g_Toggles.EspName ||
            MenuSwitches::g_Toggles.EspHealth ||
            MenuSwitches::g_Toggles.EspDistance ||
            MenuSwitches::g_Toggles.EspSkeleton ||
            MenuSwitches::g_Toggles.EspSnapline;

        if (!anyEnabled)
        {
            ImGui::TextDisabled("暂无可预览内容");
            ImGui::TextDisabled("请先在 Main 面板启用 ESP 项");
            return;
        }

        /* ============================================================
     * 函数：DrawEspShowPane —— ESP 预览面板（Show）
     *
     * 【功能说明】
     * 在菜单视觉页右侧的 "Show" 面板中，根据当前开启的 ESP 开关，
     * 绘制一个模拟的游戏实体预览效果，包括：
     *   - Box 方框（2D）
     *   - Name 名字
     *   - Health 血量条 + 百分比文字
     *   - Distance 距离
     *   - Snapline 射线/拉线
     *   - Skeleton 骨骼（完整人体骨架）
     *
     * 【颜色方案】
     * 一比一复刻 mofang/globals.cpp 中的 colorN 颜色数组：
     *   box2D_colorN      = (0.3, 0.7, 1.0) → 蓝色
     *   SnapLine_colorN   = (0.0, 1.0, 0.0) → 绿色
     *   Name_colorN       = (1.0, 0.8, 0.0) → 金色
     *   Distance_colorN   = (0.8, 0.2, 0.8) → 紫色
     *   Bone_Unobstructed = (0.5, 1.0, 0.5) → 浅绿（可见骨骼）
     *   Bone_Occlusion    = (0.5, 0.0, 0.5) → 深紫（遮挡骨骼）
     *
     * 【扩展指南】
     * 如果要添加新的预览元素：
     *   1. 在 MenuSwitches.h 的 ToggleState 中新增 bool 字段
     *   2. 在 MenuSwitches.cpp 中确保 ResetDefaults 覆盖它
     *   3. 在本函数中添加 if (开关) { 绘制代码 }
     *   4. 在 DrawESPTab() 中添加对应的 ImGui::Checkbox
     * ============================================================ */
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImVec2 boxMin(p.x + 38.0f, p.y + 16.0f);
    ImVec2 boxMax(p.x + 188.0f, p.y + 236.0f);

    // === mofang 风格颜色常量（取自 mofang/globals.cpp） ===
    const ImU32 boxColor      = IM_COL32(76,  178, 255, 255); // box2D_colorN: 蓝色边框
    const ImU32 boxFill       = IM_COL32(76,  178, 255,  50); // box2D 半透明填充
    const ImU32 snaplineColor = IM_COL32(0,  255,   0, 255); // SnapLine_colorN: 绿色射线
    const ImU32 nameColor     = IM_COL32(255, 204,   0, 255); // Name_colorN: 金色名字
    const ImU32 distanceColor = IM_COL32(204, 51,  204, 255); // Distance_colorN: 紫色距离
    const ImU32 hpGreen       = IM_COL32(60,  235,  85, 255); // 血量条绿色
    const ImU32 hpBg          = IM_COL32(40,   40,  40, 220); // 血量条背景深灰
    const ImU32 boneColor     = IM_COL32(128, 255, 128, 255); // Bone_Unobstructed: 可见骨骼浅绿
    const ImU32 boneOccluded  = IM_COL32(128,   0, 128, 255); // Bone_Occlusion: 遮挡骨骼深紫

    const float cx = (boxMin.x + boxMax.x) * 0.5f;
    const float boxHeight = boxMax.y - boxMin.y;

    // ===== 1. ESP Box（2D 方框预览） =====
    if (MenuSwitches::g_Toggles.EspBox)
    {
        dl->AddRectFilled(boxMin, boxMax, boxFill);          // 半透明蓝色填充
        dl->AddRect(boxMin, boxMax, boxColor, 0.0f, 0, 2.0f); // 蓝色边框，2px 粗细
    }

    // ===== 2. ESP Name（名字预览） =====
    if (MenuSwitches::g_Toggles.EspName)
        dl->AddText(ImVec2(boxMin.x + 2.0f, boxMin.y - 20.0f), nameColor, "Zombie_BP_01");

    // ===== 3. ESP Health（血量预览） =====
    // 左侧竖条显示血量，并附百分比文字
    if (MenuSwitches::g_Toggles.EspHealth)
    {
        const float hpBarWidth = 4.0f;
        const ImVec2 hpBgMin(boxMin.x - 8.0f, boxMin.y);
        const ImVec2 hpBgMax(boxMin.x - 8.0f + hpBarWidth, boxMax.y);
        const float hpPercent = 0.66f;  // 模拟 66% 血量
        const float hpHeight = boxHeight * hpPercent;

        dl->AddRectFilled(hpBgMin, hpBgMax, hpBg);      // 深灰背景
        dl->AddRectFilled(                               // 绿色血量从底部向上
            ImVec2(hpBgMin.x, boxMax.y - hpHeight),
            ImVec2(hpBgMax.x, boxMax.y),
            hpGreen
        );
        dl->AddRect(hpBgMin, hpBgMax, IM_COL32(0, 0, 0, 180), 0.0f, 0, 1.0f); // 黑色边框

        char hpText[16];
        sprintf_s(hpText, "66%%");
        dl->AddText(ImVec2(hpBgMax.x + 3.0f, boxMax.y - hpHeight - 4.0f), hpGreen, hpText);
    }

    // ===== 4. ESP Distance（距离预览） =====
    if (MenuSwitches::g_Toggles.EspDistance)
        dl->AddText(ImVec2(boxMin.x, boxMax.y + 8.0f), distanceColor, "126m");

    // ===== 5. ESP Snapline（射线/拉线预览） =====
    // 从 box 底部中心延伸到屏幕下方
    if (MenuSwitches::g_Toggles.EspSnapline)
        dl->AddLine(ImVec2(cx, boxMax.y), ImVec2(p.x + 132.0f, p.y + 286.0f), snaplineColor, 1.5f);

    // ===== 6. ESP Skeleton（骨骼预览） =====
    // 完整人体骨架：躯干4段 + 双臂(肩肘手) + 双腿(膝足)
    // 绿色 = 可见骨骼，紫色 = 遮挡骨骼（前臂模拟遮挡）
    if (MenuSwitches::g_Toggles.EspSkeleton)
    {
        const float headY   = boxMin.y + 24.0f;  // 头部 Y
        const float neckY   = boxMin.y + 44.0f;  // 颈部 Y
        const float chestY  = boxMin.y + 80.0f;  // 胸部 Y
        const float pelvisY = boxMax.y - 32.0f;  // 骨盆 Y
        const float footY   = boxMax.y - 8.0f;   // 脚部 Y

        // --- 躯干（Spine）---
        dl->AddLine(ImVec2(cx, headY),  ImVec2(cx, neckY),  boneColor, 1.8f);  // 头 → 颈
        dl->AddLine(ImVec2(cx, neckY),  ImVec2(cx, chestY), boneColor, 1.8f);  // 颈 → 胸
        dl->AddLine(ImVec2(cx, chestY), ImVec2(cx, pelvisY), boneColor, 1.8f); // 胸 → 骨盆

        // --- 左臂（Left Arm）---
        dl->AddLine(ImVec2(cx, neckY + 8.0f), ImVec2(cx - 30.0f, neckY + 32.0f), boneColor, 1.5f);     // 肩 → 肘（可见）
        dl->AddLine(ImVec2(cx - 30.0f, neckY + 32.0f), ImVec2(cx - 26.0f, neckY + 48.0f), boneOccluded, 1.5f); // 肘 → 手（遮挡）

        // --- 右臂（Right Arm）---
        dl->AddLine(ImVec2(cx, neckY + 8.0f), ImVec2(cx + 30.0f, neckY + 32.0f), boneColor, 1.5f);     // 肩 → 肘（可见）
        dl->AddLine(ImVec2(cx + 30.0f, neckY + 32.0f), ImVec2(cx + 26.0f, neckY + 48.0f), boneOccluded, 1.5f); // 肘 → 手（遮挡）

        // --- 左腿（Left Leg）---
        dl->AddLine(ImVec2(cx, pelvisY), ImVec2(cx - 15.0f, pelvisY + 32.0f), boneColor, 1.8f);   // 骨盆 → 膝
        dl->AddLine(ImVec2(cx - 15.0f, pelvisY + 32.0f), ImVec2(cx - 12.0f, footY), boneColor, 1.8f); // 膝 → 足

        // --- 右腿（Right Leg）---
        dl->AddLine(ImVec2(cx, pelvisY), ImVec2(cx + 15.0f, pelvisY + 32.0f), boneColor, 1.8f);   // 骨盆 → 膝
        dl->AddLine(ImVec2(cx + 15.0f, pelvisY + 32.0f), ImVec2(cx + 12.0f, footY), boneColor, 1.8f); // 膝 → 足
    }

    // 预留区域高度，避免下方 "Other" 面板重叠。
    ImGui::Dummy(ImVec2(0.0f, 292.0f));
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

        /* 拐角方框 (Corner Box)
         * 开启后只绘制方框的四个角，中间线条消失。
         * 视觉更简洁，不遮挡目标身体。 */
        ImGui::Checkbox("拐角方框", &MenuSwitches::g_Toggles.EspCornerBox);

        /* ESP 快速颜色 (Quick Color)
         * 在此处快速调节方框颜色，更详细的颜色配置在"颜色"页面。 */
        ImGui::ColorEdit4("方框颜色(快速)", MenuSwitches::g_Toggles.EspColor, ImGuiColorEditFlags_NoInputs);

        // ---- 空行留白，占满面板，保持布局稳定 ----
        ImGui::TextDisabled("颜色详细配置请到 颜色 页面");
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

        /* 僵尸目标 (Zombie Filter)
         * 勾选后 ESP 对僵尸类型敌人生效。
         * 僵尸通常移动有规律，适合开启方框追踪。 */
        ImGui::Checkbox("僵尸目标", &MenuSwitches::g_Toggles.FilterZombie);

        /* 吸血鬼目标 (Vampire Filter)
         * 勾选后 ESP 对吸血鬼类型敌人生效。
         * 吸血鬼通常移动更快，骨骼瞄准更推荐。 */
        ImGui::Checkbox("吸血鬼目标", &MenuSwitches::g_Toggles.FilterVampire);

        /* 忽略队友 (Ignore Teammate)
         * 开启后不显示队友的 ESP 信息。
         * 避免屏幕上出现过多无用标记。 */
        ImGui::Checkbox("忽略队友", &MenuSwitches::g_Toggles.FilterTeammate);

        /* 仅可见目标 (Visible Only)
         * 开启后只渲染玩家视野范围内可见的目标。
         * 被墙壁等遮挡的敌人不显示 ESP。 */
        ImGui::Checkbox("仅可见目标", &MenuSwitches::g_Toggles.FilterVisibleOnly);

        /* 过滤死亡目标 (Filter Dead)
         * 开启后排除已经死亡的目标，只有存活敌人显示 ESP。 */
        ImGui::Checkbox("过滤死亡目标", &MenuSwitches::g_Toggles.FilterDead);
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

        /* 显示 FPS (Show FPS)
         * 在屏幕角落显示当前帧率。
         * 用于判断性能影响程度。 */
        ImGui::Checkbox("显示 FPS 计数器", &MenuSwitches::g_Toggles.ShowFps);

        ImGui::Spacing();

        // ---- UI 参数 (UI) ----
        ImGui::SeparatorText("界面 - UI");

        /* 菜单透明度 (Menu Alpha)
         * 控制整个菜单窗口的不透明度。
         * 值越小越透明，便于在菜单开启时仍看到游戏画面。 */
        ImGui::SliderFloat("菜单透明度", &MenuSwitches::g_Toggles.MenuAlpha, 0.5f, 1.0f, "%.2f");

        /* 主题选择 (Theme Selection)
         * 切换菜单的暗色/亮色主题。
         * Dark = 深色主题（推荐游戏内使用）
         * Light = 浅色主题 */
        ImGui::Combo("主题选择", &MenuSwitches::g_Toggles.ThemeIndex, "深色主题\0浅色主题\0\0");

        /* UI 缩放 (UI Scale)
         * 整体菜单元素的大小缩放比例。
         * 高分辨率屏幕可适当调大(1.2-1.5)。
         * 低分辨率屏幕可调小(0.8-1.0)。 */
        ImGui::SliderFloat("UI缩放比例", &MenuSwitches::g_Toggles.UiScale, 0.8f, 1.5f, "%.2f");

        ImGui::Spacing();

        // ---- 性能选项 (Performance) ----
        ImGui::SeparatorText("性能 - Performance");

        /* 最大 FPS 限制 (Max FPS)
         * 限制菜单/覆盖层的最大帧率。
         * 较低的值可以减少 CPU/GPU 占用。
         * 0 = 不限制（默认），60-144 适合大多数场景。 */
        ImGui::SliderFloat("最大 FPS", &MenuSwitches::g_Toggles.MaxFps, 30.0f, 240.0f, "%.0f FPS");

        /* 垂直同步 (VSync)
         * 开启后菜单刷新与显示器刷新率同步。
         * 可以减少画面撕裂，但可能增加输入延迟。 */
        ImGui::Checkbox("垂直同步", &MenuSwitches::g_Toggles.Vsync);

        /* 性能模式 (Performance Mode)
         * 开启后禁用粒子/渐变动画等非必要特效。
         * 建议低配电脑或需要最大 FPS 时开启。 */
        ImGui::Checkbox("性能模式", &MenuSwitches::g_Toggles.PerformanceMode);
        ImGui::SameLine();
        ImGui::TextDisabled("关闭粒子/特效以提升帧率");

        ImGui::Spacing();

        // ---- 安全与控制 (Security) ----
        ImGui::SeparatorText("安全 - Security");

        /* 流媒体隐藏 (Stream Proof)
         * 开启后菜单在 OBS/直播软件采集时自动隐藏。
         * 适合直播或录屏场景，保护隐私。 */
        ImGui::Checkbox("流媒体隐藏", &MenuSwitches::g_Toggles.StreamProof);

        /* 紧急键 (Panic Key)
         * 开启后允许使用紧急快捷键（默认 Delete/Home）
         * 立即关闭菜单和所有覆盖层。 */
        ImGui::Checkbox("启用紧急键", &MenuSwitches::g_Toggles.PanicKeyEnabled);

        ImGui::Spacing();

        // 预留调用：应用 UI 与主题参数
        if (ImGui::Button("应用 UI 参数(预留接口)", ImVec2(220, 0)))
        {
            const bool ok = MenuFeatureStubs::ApplyThemeConfig(
                MenuSwitches::g_Toggles.AccentHue,
                MenuSwitches::g_Toggles.MenuAlpha,
                MenuSwitches::g_Toggles.UiScale);
            PushLog(ok ? "UI 参数应用成功" : MenuFeatureStubs::NotImplementedMessage());
        }

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
        ImGui::Checkbox("运行时自动保存配置", &MenuSwitches::g_Toggles.AutoSavePreview);

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
        ImGui::Spacing();

        ImGui::SeparatorText("武器修改 - Weapon");

        ImGui::Checkbox("无后坐力", &MenuSwitches::g_Toggles.PreviewNoRecoil);
        ImGui::Checkbox("快速射击", &MenuSwitches::g_Toggles.PreviewRapidFire);
        ImGui::Checkbox("无散布", &MenuSwitches::g_Toggles.PreviewNoSpread);
        ImGui::Checkbox("无限弹药", &MenuSwitches::g_Toggles.PreviewInfiniteAmmo);

        ImGui::Spacing();
        ImGui::SeparatorText("角色修改 - Player");

        ImGui::Checkbox("移动加速", &MenuSwitches::g_Toggles.PreviewSpeedHack);
        ImGui::SliderFloat("速度倍率", &MenuSwitches::g_Toggles.PreviewSpeedScale, 1.0f, 3.0f, "%.2f 倍");
        ImGui::SliderInt("弹药值", &MenuSwitches::g_Toggles.PreviewAmmoValue, 30, 9999);

        ImGui::Spacing();
        ImGui::SeparatorText("世界 - World");

        ImGui::Checkbox("显示世界对象列表", &MenuSwitches::g_Toggles.PreviewWorldObjects);
        ImGui::Checkbox("显示实体信息", &MenuSwitches::g_Toggles.PreviewEntityInfo);

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

    // ==================== 主渲染函数 ====================

    void Render()
    {
        if (!IsMenuOpen)
            return;

        static float tab_size = 0.f;
        static float arrow_roll = 0.f;
        static bool tab_opening = true;

        // 左侧 tab 区展开/收起动画。
        tab_size = ImLerp(tab_size, tab_opening ? 160.f : 0.f, ImGui::GetIO().DeltaTime * 12.f);
        arrow_roll = ImLerp(arrow_roll, tab_opening && (tab_size >= 159) ? 1.f : -1.f, ImGui::GetIO().DeltaTime * 12.f);

        ImGuiStyle* s = &ImGui::GetStyle();

        s->WindowPadding = ImVec2(0, 0), s->WindowBorderSize = 0;
        s->ItemSpacing = ImVec2(20, 20);
        s->ScrollbarSize = 8.f;

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

        ImGui::SetNextWindowSize(ImVec2(c::bg::size) + ImVec2(tab_size, 0));

        ImGui::Begin("IMGUI MENU", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus);
        {
            const ImVec2& pos = ImGui::GetWindowPos();
            const ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
           
            ImGui::GetBackgroundDrawList()->AddRectFilled(pos, pos + ImVec2(c::bg::size) + ImVec2(tab_size, 0), ImGui::GetColorU32(c::bg::background), c::bg::rounding);
            ImGui::GetBackgroundDrawList()->AddRect(pos, pos + ImVec2(c::bg::size) + ImVec2(tab_size, 0), ImGui::GetColorU32(c::bg::outline_background), c::bg::rounding);

            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(c::accent_text_color));

            if (font::pTahomaBold2) ImGui::PushFont(font::pTahomaBold2);
            ImGui::RenderTextClipped(pos + ImVec2(60, 0) + spacing, pos + spacing + ImVec2(60, 60) + ImVec2(tab_size + (spacing.x / 2) - 30, 0), "AndThen", NULL, NULL, ImVec2(0.5f, 0.5f), NULL); 
            if (font::pTahomaBold2) ImGui::PopFont();

            ImGui::RenderTextClipped(pos + ImVec2(60 + spacing.x, c::bg::size.y - 60 * 2), pos + spacing + ImVec2(60, c::bg::size.y) + ImVec2(tab_size, 0), "剩余时间_2099-9-9", NULL, NULL, ImVec2(0.0f, 0.43f), NULL);
            ImGui::RenderTextClipped(pos + ImVec2(60 + spacing.x, c::bg::size.y - 60 * 2), pos + spacing + ImVec2(60, c::bg::size.y) + ImVec2(tab_size, 0), "然后\n", NULL, NULL, ImVec2(0.0f, 0.57f), NULL);

            if (font::pTahomaBold2) ImGui::PushFont(font::pTahomaBold2); 
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(250, 255, 255, 255));
            ImGui::RenderTextClipped(pos + ImVec2(0, 0) + spacing, pos + spacing + ImVec2(60, 40) + ImVec2(tab_size + (spacing.x / 2) + 199, 0), "Hello, User-241013", NULL, NULL, ImVec2(1.f, 0.5f), NULL); 
            ImGui::PopStyleColor();
            if (font::pTahomaBold2) ImGui::PopFont(); 

            if (image::logo)
                ImGui::GetBackgroundDrawList()->AddImage(image::logo, pos + ImVec2(14, 14), pos + ImVec2(46, 46), ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255));

            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(90, 93, 100, 255)); 
            ImGui::RenderTextClipped(pos + ImVec2(30, 20) + spacing, pos + spacing + ImVec2(60, 80) + ImVec2(tab_size + (spacing.x / 2) + 108, -20), "欢迎回来!", NULL, NULL, ImVec2(1.f, 0.5f), NULL); 
            ImGui::PopStyleColor();

            static char Search[64] = { "" };
            // 顶部搜索框目前仅占位，不参与过滤逻辑。
            ImGui::SetCursorPos(ImVec2(385 + tab_size, -20) + (s->ItemSpacing * 2));
            ImGui::BeginChild("", " ", ImVec2((c::bg::size.x - 60 - s->ItemSpacing.x * 4) / 2, 60));
            if (font::pTabIcon) ImGui::PushFont(font::pTabIcon);
            ImGui::Text("I"); ImGui::SameLine(); ImGui::Text("H"); ImGui::SameLine(); ImGui::Text("G");
            if (font::pTabIcon) ImGui::PopFont(); 
            ImGui::SameLine();
            ImGui::SetNextItemWidth(180);
            ImGui::InputTextWithHint("查询功能", "暂未实现...", Search, 64, NULL);
            ImGui::EndChild();
            ImGui::PopStyleColor(1);

            const char* nametab_array1[6] = { "E", "D", "A", "G", "C", "B" };
            const char* nametab_array[6] = { "瞄准", "视觉", "颜色", "内存", "设置", "配置" };
            const char* nametab_hint_array[6] = { "Aim, Recoil, Trigger", "Overlay, Chams, World", "Game Colors", "World Memory", "Element Setup", "Save Settings" };

            static int tabs = 0;

            ImGui::SetCursorPos(ImVec2(spacing.x + (60 - 22) / 2, 60 + (spacing.y * 2) + 22));
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

            ImGui::GetBackgroundDrawList()->AddRectFilled(pos + ImVec2(0, -20 + spacing.y) + spacing, pos + spacing + ImVec2(60, c::bg::size.y - 60 - spacing.y * 3) + ImVec2(tab_size, 0), ImGui::GetColorU32(c::child::background), c::child::rounding);
            ImGui::GetBackgroundDrawList()->AddRect(pos + ImVec2(0, -20 + spacing.y) + spacing, pos + spacing + ImVec2(60, c::bg::size.y - 60 - spacing.y * 3) + ImVec2(tab_size, 0), ImGui::GetColorU32(c::child::outline_background), c::child::rounding);

            ImGui::GetBackgroundDrawList()->AddRectFilled(pos + ImVec2(0, c::bg::size.y - 60 - spacing.y * 2) + spacing, pos + spacing + ImVec2(60, c::bg::size.y - spacing.y * 2) + ImVec2(tab_size, 0), ImGui::GetColorU32(c::child::background), c::child::rounding);
            ImGui::GetBackgroundDrawList()->AddRect(pos + ImVec2(0, c::bg::size.y - 60 - spacing.y * 2) + spacing, pos + spacing + ImVec2(60, c::bg::size.y - spacing.y * 2) + ImVec2(tab_size, 0), ImGui::GetColorU32(c::child::outline_background), c::child::rounding);

            if (image::logo)
                ImGui::GetWindowDrawList()->AddImage(image::logo, pos + ImVec2(0, c::bg::size.y - 60 - spacing.y * 2) + spacing + ImVec2(12, 12), pos + spacing + ImVec2(60, c::bg::size.y - spacing.y * 2) - ImVec2(12, 12), ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255));

            ImGui::GetWindowDrawList()->AddCircleFilled(pos + ImVec2(63, c::bg::size.y - (spacing.y * 2) + 3), 10.f, ImGui::GetColorU32(c::child::background), 100);
            ImGui::GetWindowDrawList()->AddCircleFilled(pos + ImVec2(63, c::bg::size.y - (spacing.y * 2) + 3), 6.f, ImColor(0, 255, 0, 255), 100);

            Particles();

            static float tab_alpha = 0.f; static float tab_add = 0.f; static int active_tab = 0;

            tab_alpha = ImClamp(tab_alpha + (4.f * ImGui::GetIO().DeltaTime * (tabs == active_tab ? 1.f : -1.f)), 0.f, 1.f);
            tab_add = ImClamp(tab_add + (std::round(350.f) * ImGui::GetIO().DeltaTime * (tabs == active_tab ? 1.f : -1.f)), 0.f, 1.f);

            if (tab_alpha == 0.f && tab_add == 0.f) active_tab = tabs;

            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, tab_alpha * s->Alpha);

            if (tabs == 0)
            {
                // Aim 页布局：主区域 + 底部两个次区域。
                ImGui::SetCursorPos(ImVec2(60 + tab_size, 60) + (s->ItemSpacing * 2));
                ImGui::BeginGroup();
                {
                    ImGui::BeginChild("D", "Main", ImVec2((c::bg::size.x - 60 - s->ItemSpacing.x * 4) + 18, 405));
                    {
                        DrawAimbotTab();
                    }
                    ImGui::EndChild();
                }
                ImGui::EndGroup();
                
                ImGui::SameLine();
                ImGui::BeginGroup();
                {
                    ImGui::SetCursorPos(ImVec2(60 + tab_size, 480) + (s->ItemSpacing * 2));
                    ImGui::BeginChild("A", "Other", ImVec2((c::bg::size.x - 60 - s->ItemSpacing.x * 4) / 2, 100));
                    {
                        // Aim_bot02 data
                    }
                    ImGui::EndChild();

                    ImGui::SetCursorPos(ImVec2(380 + tab_size, 480) + (s->ItemSpacing * 2));
                    ImGui::BeginChild("E", "Setting", ImVec2((c::bg::size.x - 60 - s->ItemSpacing.x * 4) / 2, 100));
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
                ImGui::SetCursorPos(ImVec2(60 + tab_size, 60) + (s->ItemSpacing * 2));
                ImGui::BeginGroup();
                {
                    ImGui::BeginChild("D", "Main", ImVec2((c::bg::size.x - 60 - s->ItemSpacing.x * 4) / 2, 420));
                    {
                        DrawESPTab();
                    }
                    ImGui::EndChild();
                }
                ImGui::EndGroup();
                
                ImGui::SameLine();
                ImGui::BeginGroup();
                {
                    ImGui::BeginChild("E", "Show", ImVec2((c::bg::size.x - 60 - s->ItemSpacing.x * 4) / 2, 420));
                    {
                        DrawEspShowPane();
                    }
                    ImGui::EndChild();

                    ImGui::BeginChild("B", "Other", ImVec2((c::bg::size.x - 60 - s->ItemSpacing.x * 4) / 2, 80));
                    {
                        DrawEspOtherPane();
                    }
                    ImGui::EndChild();

                    ImGui::SetCursorPos(ImVec2(60 + tab_size, 500) + (s->ItemSpacing * 2));
                    ImGui::BeginChild("A", "Filter ", ImVec2((c::bg::size.x - 60 - s->ItemSpacing.x * 4) / 2, 80));
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
                ImGui::SetCursorPos(ImVec2(60 + tab_size, 60) + (s->ItemSpacing * 2));
                ImGui::BeginGroup();
                {
                    ImGui::BeginChild("D", "Main", ImVec2((c::bg::size.x - 60 - s->ItemSpacing.x * 4) + 18, 445));
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
                ImGui::SetCursorPos(ImVec2(60 + tab_size, 60) + (s->ItemSpacing * 2));
                ImGui::BeginGroup();
                {
                    ImGui::BeginChild("D", "Main", ImVec2((c::bg::size.x - 60 - s->ItemSpacing.x * 4) + 18, 470));
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
                ImGui::SetCursorPos(ImVec2(60 + tab_size, 60) + (s->ItemSpacing * 2));
                ImGui::BeginGroup();
                {
                    ImGui::BeginChild("D", "Main", ImVec2((c::bg::size.x - 60 - s->ItemSpacing.x * 4) + 18, 495));
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
                ImGui::SetCursorPos(ImVec2(60 + tab_size, 60) + (s->ItemSpacing * 2));
                ImGui::BeginGroup();
                {
                    ImGui::BeginChild("D", "Main", ImVec2((c::bg::size.x - 60 - s->ItemSpacing.x * 4) + 18, 520));
                    {
                        DrawConfigTab();
                    }
                    ImGui::EndChild();
                }
                ImGui::EndGroup();
            }

            ImGui::PopStyleVar();
        }
        ImGui::End();

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
