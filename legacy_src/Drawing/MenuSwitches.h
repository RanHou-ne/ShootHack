#pragma once

namespace MenuSwitches
{
    // 菜单开关总状态。
    // 设计目标：
    // 1) 把所有 UI 勾选/滑块值集中管理，避免状态散落在多个 cpp 静态变量里。
    // 2) 后续接配置文件读写时，只需序列化/反序列化这一份结构。
    // 3) 新增选项时，优先在这里加字段，再到 Menu.cpp 绑定控件。
    struct ToggleState
    {
        // ===== 自瞄页（Aim） =====
        // 是否启用自瞄总开关。
        bool OpenAimbot = false;
        // 骨骼瞄准（头/胸等骨骼点）的预留开关。
        bool BoneOn = false;
        // 静默自瞄预留开关（当前仅 UI 状态，不执行游戏逻辑）。
        bool MemoryAimbot = false;
        // 穿透判定相关开关（当前仅 UI 状态）。
        bool Missed_shot = false;
        // 自瞄范围，单位按当前 UI 展示逻辑理解即可（未接入真实算法）。
        float Aim_Range = 120.0f;
        // 自瞄 XY 阈值/平滑阈值预留参数。
        float threshold = 8.0f;
        // 扳机功能预览开关（预留接口）。
        bool TriggerBot = false;
        // 预测瞄准预览开关（预留接口）。
        bool AimPrediction = false;
        // FOV 限制预览开关（预留接口）。
        bool AimFovLimit = true;
        // 自瞄 FOV 角度。
        float AimFov = 35.0f;
        // 自瞄平滑（数值越大越慢）。
        float AimSmooth = 5.0f;
        // 瞄准骨骼索引（0:头 1:胸 2:骨盆）。
        int AimBoneIndex = 0;

        // ===== 视觉页（ESP） =====
        // 以下为占位状态，后续接 ESP 绘制时直接读取。
        bool EspBox = false;
        bool EspName = false;
        bool EspHealth = false;
        bool EspDistance = false;
        float EspThickness = 1.5f;
        // 骨骼绘制预览开关。
        bool EspSkeleton = false;
        // 射线（玩家到目标连线）预览开关。
        bool EspSnapline = false;
        // 物资显示预览开关。
        bool EspLoot = false;
        // 小地图雷达预览开关。
        bool EspRadar = false;
        // ESP 最大距离。
        float EspMaxDistance = 250.0f;
        // 文本缩放。
        float EspTextScale = 1.0f;
        // 颜色与样式。
        float EspColor[4] = { 1.0f, 0.55f, 0.30f, 1.0f };
        bool EspFillBox = false;
        bool EspCornerBox = false;
        // 过滤条件（Filter 面板）。
        bool FilterZombie = true;
        bool FilterVampire = true;
        bool FilterTeammate = false;
        bool FilterVisibleOnly = false;
        bool FilterDead = false;

        // ===== 颜色页（Color） =====
        // 主题色调（HSV H 通道，0~1）。
        float AccentHue = 0.06f;
        // 透明度倍率。
        float GlobalAlpha = 0.95f;
        // 是否启用动态渐变。
        bool DynamicGradient = false;

        // ---- ESP 元素颜色数组（RGBA float[4]），供给 ColorEdit4 使用 ----
        // 2D 方框颜色 (mofang box2D_colorN: 0.3,0.7,1.0,1.0 → 蓝色)
        float ColorBox2D[4] = { 0.30f, 0.70f, 1.00f, 1.00f };
        // 3D 方框颜色
        float ColorBox3D[4] = { 0.20f, 0.60f, 0.90f, 1.00f };
        // 名字颜色 (mofang Name_colorN: 1.0,0.8,0.0,1.0 → 金色)
        float ColorName[4] = { 1.00f, 0.80f, 0.00f, 1.00f };
        // 射线颜色 (mofang SnapLine_colorN: 0.0,1.0,0.0,1.0 → 绿色)
        float ColorSnapline[4] = { 0.00f, 1.00f, 0.00f, 1.00f };
        // 距离文字颜色 (mofang Distance_colorN: 0.8,0.2,0.8,1.0 → 紫色)
        float ColorDistance[4] = { 0.80f, 0.20f, 0.80f, 1.00f };
        // 血量条颜色 (默认绿色)
        float ColorHealth[4] = { 0.24f, 0.92f, 0.33f, 1.00f };
        // 可见骨骼颜色 (mofang Bone_Unobstructed: 0.5,1.0,0.5,1.0 → 浅绿)
        float ColorBoneVisible[4] = { 0.50f, 1.00f, 0.50f, 1.00f };
        // 遮挡骨骼颜色 (mofang Bone_Occlusion: 0.5,0.0,0.5,1.0 → 深紫)
        float ColorBoneOccluded[4] = { 0.50f, 0.00f, 0.50f, 1.00f };
        // 雷达颜色 (默认浅蓝色)
        float ColorRadar[4] = { 0.30f, 0.80f, 1.00f, 1.00f };
        // 物品高亮颜色 (默认橙色)
        float ColorLoot[4] = { 1.00f, 0.65f, 0.00f, 1.00f };

        // ===== 内存页（Memory） =====
        // 以下全部为“能力预览开关”，仅展示潜在功能，不执行实际写内存。
        bool PreviewNoRecoil = false;
        bool PreviewRapidFire = false;
        bool PreviewNoSpread = false;
        bool PreviewSpeedHack = false;
        bool PreviewInfiniteAmmo = false;
        float PreviewSpeedScale = 1.2f;
        int PreviewAmmoValue = 999;
        // 显示世界对象列表
        bool PreviewWorldObjects = false;
        // 显示实体信息
        bool PreviewEntityInfo = false;

        // ===== 设置页（Setting） =====
        // 工具提示显示。
        bool ShowTooltips = true;
        // FPS 显示开关。
        bool ShowFps = true;
        // 菜单透明度（建议范围 0.5~1.0）。
        float MenuAlpha = 0.95f;
        // 主题索引（0: Dark, 1: Light）。
        int ThemeIndex = 0;
        // FPS 上限占位参数。
        float MaxFps = 144.0f;
        // 垂直同步开关占位参数。
        bool Vsync = false;
        // 性能模式开关（默认开启）。
        // 开启后：降低菜单特效开销，优先保证帧率稳定。
        bool PerformanceMode = true;
        // 一键隐藏菜单（流媒体安全）预览开关。
        bool StreamProof = false;
        // 紧急键启用预览（用于快速关闭全部功能）。
        bool PanicKeyEnabled = true;
        // UI 缩放。
        float UiScale = 1.0f;

        // ===== 配置页（Config） =====
        // 自动保存预览开关。
        bool AutoSavePreview = false;
        // 启动自动读取预览开关。
        bool AutoLoadPreview = false;
    };

    // 全局状态实例。
    // Menu.cpp 中所有控件都应绑定到这个对象，避免重复状态源。
    extern ToggleState g_Toggles;

    // 恢复默认值。
    // 常见调用时机：
    // - 点击“重置配置”按钮
    // - 配置文件加载失败后的兜底
    // - 功能初始化时需要干净状态
    void ResetDefaults();
}
