// ============================================================================
// 文件：src/UI/Menu/MenuSwitches.h
// 模块：UI 层 - 菜单全局状态结构体
// ============================================================================
// 【用途】
//   集中管理所有菜单 UI 控件的状态（开关、滑条值、颜色等）。
//   所有 ImGui 控件直接绑定到 g_Toggles 的字段，
//   配置系统（Config.cpp）也通过成员指针读写这些字段。
//
// 【设计原则】
//   - 单一数据源：所有 UI 状态集中在 ToggleState 结构体中
//   - 易于序列化：字段类型均为 bool/float/int/float[4]，直接映射 JSON
//   - 易于扩展：新增功能只需在结构体中添加字段
//
// ============================================================================
// 【扩展指南】
//
//   ① 新增一个布尔开关（如"无限血量"）：
//      a. 在 ToggleState 中添加：
//            bool InfiniteHealth = false;  // 无限血量开关
//      b. 在 ConfigSchema.h 的 CFG_BOOL_ITEMS 中添加：
//            CFG_BOOL("InfiniteHealth", InfiniteHealth, false, "无限血量")
//      c. 在 Menu.cpp 对应 Tab 中添加 ImGui 控件：
//            ImGui::Checkbox("无限血量", &MenuSwitches::g_Toggles.InfiniteHealth);
//
//   ② 新增一个浮点参数（如"子弹速度倍率"）：
//      a. 在 ToggleState 中添加：
//            float BulletSpeedScale = 1.0f;  // 子弹速度倍率
//      b. 在 ConfigSchema.h 的 CFG_FLOAT_ITEMS 中添加对应宏
//      c. 在 Menu.cpp 中添加 SliderFloat 控件
//
//   ③ 新增一个颜色（如"爆头特效颜色"）：
//      a. 在 ToggleState 中添加：
//            float ColorHeadshot[4] = {1.0f, 0.0f, 0.0f, 1.0f};  // 爆头特效颜色
//      b. 在 ConfigSchema.h 的 CFG_COLOR_ITEMS 中添加对应宏
//      c. 在 Menu.cpp 的颜色页中添加 ColorEdit4 控件
//
// ============================================================================

#pragma once

namespace MenuSwitches
{
    // ========================================================================
    // ToggleState —— 所有菜单控件的状态集合
    // ========================================================================
    // 字段分组：
    //   - 自瞄页（Aim）
    //   - 视觉页（ESP）
    //   - 颜色页（Color）
    //   - 内存功能页（Memory）
    //   - 设置页（Settings）
    //   - 配置页（Config）
    struct ToggleState
    {
        // ====================================================================
        // 自瞄页（Aim）
        // ====================================================================

        bool  OpenAimbot    = false;   // 自瞄总开关。打开后自瞄功能开始工作。
        bool  BoneOn        = false;   // 骨骼瞄准。开启后瞄准骨骼位置（头/胸等）。
        bool  MemoryAimbot  = false;   // 静默自瞄。客户端不显示准星移动但子弹命中。
        bool  Missed_shot   = false;   // 穿透自瞄。无视障碍物瞄准敌人。
        float Aim_Range     = 120.0f;  // 自瞄最大范围（米）。超出此距离不触发自瞄。
        float threshold     = 8.0f;   // XY 偏移阈值。准星与目标偏移小于此值时触发修正。
        bool  TriggerBot    = false;   // 扳机开火。准星移到敌人上自动开枪。
        bool  AimPrediction = false;   // 预测瞄准。考虑敌人移动速度提前预判位置。
        bool  AimFovLimit   = true;    // FOV 限制。只对指定角度范围内的敌人触发自瞄。
        float AimFov        = 35.0f;  // FOV 角度（度）。自瞄有效角度范围。
        float AimSmooth     = 5.0f;   // 平滑系数。值越大移动越自然，1=瞬间锁定。
        int   AimBoneIndex  = 0;      // 骨骼索引。0=头部, 1=颈部, 2=胸部。

        // ====================================================================
        // 视觉页（ESP）
        // ====================================================================

        bool  EspBox         = false;   // 方框 ESP。在目标周围绘制 2D 矩形框。
        bool  EspName        = false;   // 名字 ESP。在方框上方显示目标名称。
        bool  EspHealth      = false;   // 血量 ESP。在方框左侧显示血量条。
        bool  EspDistance    = false;   // 距离 ESP。在方框下方显示与目标的距离。
        bool  EspSkeleton    = false;   // 骨骼绘制。绘制目标内部骨骼连接线。
        bool  EspSnapline    = false;   // 射线绘制。从屏幕边缘到目标画辅助追踪线。
        bool  EspLoot        = false;   // 物资显示。标记场景中可拾取物品。
        bool  EspRadar       = false;   // 雷达显示。小地图显示敌人位置。
        float EspMaxDistance = 250.0f;  // ESP 最大显示距离（米）。
        float EspThickness   = 1.5f;   // 方框线条粗细（像素）。
        float EspTextScale   = 1.0f;   // 文字缩放比例。
        float EspColor[4]    = { 1.0f, 0.55f, 0.30f, 1.0f };  // ESP 主色调（快速颜色）

        // ---- 方框样式 ----
        bool EspFillBox   = false;  // 实心方框。方框内部半透明填充。
        bool EspCornerBox = false;  // 拐角方框。只绘制四个角，更简洁。

        // ---- 过滤条件 ----
        // bool FilterZombie      = true;   // 对僵尸类型敌人显示 ESP。
        // bool FilterVampire     = true;   // 对吸血鬼类型敌人显示 ESP。
        // bool FilterTeammate    = false;  // 忽略队友（不显示队友 ESP）。
        // bool FilterVisibleOnly = false;  // 仅可见目标（被遮挡的不显示）。
        bool FilterDead        = true;  // 过滤死亡目标（只显示存活敌人）。

        // ====================================================================
        // 颜色页（Color）
        // ====================================================================

        float AccentHue      = 0.06f;  // 主题色相（HSV H 通道，0~1）。
        float GlobalAlpha    = 0.95f;  // 全局透明度倍率。
        bool  DynamicGradient = false; // 动态渐变开关。

        // ---- ESP 元素颜色（RGBA float[4]，供 ColorEdit4 使用）----
        // 颜色值范围 0.0~1.0，对应 RGB 0~255
        float ColorBox2D[4]        = { 0.30f, 0.70f, 1.00f, 1.00f };  // 2D 方框颜色（蓝色）
        float ColorBox3D[4]        = { 0.20f, 0.60f, 0.90f, 1.00f };  // 3D 方框颜色
        float ColorName[4]         = { 1.00f, 0.80f, 0.00f, 1.00f };  // 名字颜色（金色）
        float ColorSnapline[4]     = { 0.00f, 1.00f, 0.00f, 1.00f };  // 射线颜色（绿色）
        float ColorDistance[4]     = { 0.80f, 0.20f, 0.80f, 1.00f };  // 距离颜色（紫色）
        float ColorHealth[4]       = { 0.24f, 0.92f, 0.33f, 1.00f };  // 血量颜色（绿色）
        float ColorBoneVisible[4]  = { 0.50f, 1.00f, 0.50f, 1.00f };  // 可见骨骼颜色（浅绿）
        float ColorBoneOccluded[4] = { 0.50f, 0.00f, 0.50f, 1.00f };  // 遮挡骨骼颜色（深紫）
        float ColorRadar[4]        = { 0.30f, 0.80f, 1.00f, 1.00f };  // 雷达颜色（蓝色）
        float ColorLoot[4]         = { 1.00f, 0.65f, 0.00f, 1.00f };  // 物品高亮颜色（橙色）

        // ====================================================================
        // 内存功能页（Memory）
        // ====================================================================

        bool  PreviewNoRecoil     = false;  // 无后坐力。消除武器开火后坐力。
        bool  PreviewRapidFire    = false;  // 快速射击。突破武器射速限制。
        bool  PreviewNoSpread     = false;  // 无扩散。消除子弹散布。
        bool  PreviewSpeedHack    = false;  // 移动加速。提升角色移动速度。
        bool  PreviewInfiniteAmmo = false;  // 无限弹药。弹药不减少。
        float PreviewSpeedScale   = 1.2f;  // 速度倍率（1.0=正常，2.0=两倍速）。
        int   PreviewAmmoValue    = 999;   // 弹药数值（无限弹药时设置的弹药量）。
        bool  PreviewWorldObjects = false;  // 显示世界对象列表（调试用）。
        bool  PreviewEntityInfo   = false;  // 显示实体信息（调试用）。
        bool  DrawObjectPointers  = false;  // 绘制所有GameObject指针位置（屏幕上显示地址）。
        bool  ShowEnemyESP        = false;  // 绘制敌人位置到屏幕（PVE_EnimyList）。

        // ====================================================================
        // 设置页（Settings）
        // ====================================================================

        bool  ShowTooltips    = true;    // 显示工具提示（鼠标悬停时显示说明）。
        bool  ShowFps         = true;    // 显示 FPS 计数器（主开关）。
        bool  ShowGameFps     = true;    // 显示游戏 FPS。
        bool  ShowOverlayFps  = true;    // 显示覆盖层 FPS。
        float MenuAlpha       = 0.95f;  // 菜单透明度（0.5~1.0）。
        int   ThemeIndex      = 0;      // 主题索引（0=深色, 1=浅色）。
        float MaxFps          = 144.0f; // 最大帧率限制（0=不限制）。
        bool  Vsync           = false;  // 垂直同步。
        bool  PerformanceMode = true;   // 性能模式（关闭粒子/特效）。
        bool  StreamProof     = false;  // 流媒体隐藏（OBS 采集时不显示菜单）。
        bool  PanicKeyEnabled = true;   // 紧急键启用（快捷键立即关闭菜单）。
        float UiScale         = 1.0f;  // UI 缩放比例（0.8~1.5）。

        // ====================================================================
        // 配置页（Config）
        // ====================================================================

        bool AutoSavePreview = false;  // 运行时自动保存配置。
        bool AutoLoadPreview = false;  // 启动时自动读取配置。

        // ====================================================================
        // 在此处添加新字段
        // ====================================================================
        // 示例：
        //   bool  MyNewFeature = false;       // 新功能开关
        //   float MyNewParam   = 1.0f;        // 新功能参数
        //   float MyNewColor[4] = {1,0,0,1};  // 新功能颜色
    };

    // ========================================================================
    // 全局状态实例
    // ========================================================================
    // 程序生命周期内保持有效。
    // 所有 ImGui 控件直接绑定到此实例的字段。
    extern ToggleState g_Toggles;

    // ========================================================================
    // ResetDefaults() —— 重置所有字段为默认值
    // ========================================================================
    // 使用聚合初始化恢复到结构体默认值，
    // 确保新增字段后也能自动纳入默认恢复流程。
    void ResetDefaults();

} // namespace MenuSwitches
