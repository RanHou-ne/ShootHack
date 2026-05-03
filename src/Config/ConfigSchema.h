// ============================================================================
// 文件：src/Config/ConfigSchema.h
// 模块：配置层 - 配置项定义表（X-Macro 宏表）
// ============================================================================
// 【用途】
//   在这里统一声明所有需要保存/读取的配置项。
//   新增菜单开关时，只需在下方对应的 CFG_XXX 列表中添加一行即可，
//   保存/读取/重置全部自动生效，无需修改 Config.cpp。
//
// ============================================================================
// 【宏说明】
//
//   CFG_BOOL  (json键名, 结构体字段, 默认值, "描述")
//   CFG_FLOAT (json键名, 结构体字段, 默认值, "描述")
//   CFG_INT   (json键名, 结构体字段, 默认值, "描述")
//   CFG_COLOR (json键名, 结构体字段, r, g, b, a, "描述")  ← 4 个独立 float
//
//   json键名   → 保存到 JSON 文件中的 key 名称（字符串）
//   结构体字段 → MenuSwitches::ToggleState 的成员名（不加引号）
//   默认值     → ResetToDefaults() 时恢复为该值
//   描述       → 仅用于注释说明，不参与序列化
//
// ============================================================================
// 【扩展示例】
//
//   ① 新增一个布尔开关（如"无限血量"）：
//      a. 在 MenuSwitches.h 的 ToggleState 中添加：
//            bool InfiniteHealth = false;
//      b. 在本文件 CFG_BOOL_ITEMS 中添加：
//            CFG_BOOL("InfiniteHealth", InfiniteHealth, false, "无限血量")
//      c. 在 Menu.cpp 对应 Tab 中添加 ImGui 控件：
//            ImGui::Checkbox("无限血量", &MenuSwitches::g_Toggles.InfiniteHealth);
//      d. 在 MenuFeatureStubs.cpp 中实现对应逻辑（可选）
//
//   ② 新增一个浮点参数（如"子弹速度倍率"）：
//      a. 在 MenuSwitches.h 添加：float BulletSpeedScale = 1.0f;
//      b. 在 CFG_FLOAT_ITEMS 添加：
//            CFG_FLOAT("BulletSpeedScale", BulletSpeedScale, 1.0f, "子弹速度倍率")
//      c. 在 Menu.cpp 添加 SliderFloat 控件
//
//   ③ 新增一个颜色（如"爆头特效颜色"）：
//      a. 在 MenuSwitches.h 添加：float ColorHeadshot[4] = {1.0f, 0.0f, 0.0f, 1.0f};
//      b. 在 CFG_COLOR_ITEMS 添加：
//            CFG_COLOR("ColorHeadshot", ColorHeadshot, 1.0f, 0.0f, 0.0f, 1.0f, "爆头特效颜色")
//      注意：颜色值必须用 4 个独立 float，不能用花括号 {r,g,b,a}，
//            因为花括号内的逗号会被预处理器误解为宏参数分隔符。
//
// ============================================================================

#pragma once

#include "UI/Menu/MenuSwitches.h"

// ============================================================================
// 内部辅助结构体（每种类型一个条目）
// ============================================================================

struct CfgBoolEntry
{
    const char* key;                                  // JSON 键名
    bool MenuSwitches::ToggleState::*fieldPtr;        // 指向 g_Toggles 成员的指针
    bool defaultValue;                                // 重置时的默认值
};

struct CfgFloatEntry
{
    const char* key;
    float MenuSwitches::ToggleState::*fieldPtr;
    float defaultValue;
};

struct CfgIntEntry
{
    const char* key;
    int MenuSwitches::ToggleState::*fieldPtr;
    int defaultValue;
};

struct CfgColorEntry
{
    const char* key;
    float (MenuSwitches::ToggleState::*fieldPtr)[4];  // 指向 float[4] 数组的成员指针
    float defaultValue[4];
};

// ============================================================================
// ↓↓↓ 配置项列表 ↓↓↓
// 新增配置项只需在对应区域加一行宏即可！
// ============================================================================

// ---- 布尔型开关 ----
// 格式：CFG_BOOL("JSON键名", 结构体字段名, 默认值, "描述")
#define CFG_BOOL_ITEMS \
    /* ===== 自瞄页 ===== */ \
    CFG_BOOL("OpenAimbot",          OpenAimbot,          false, "自瞄总开关") \
    CFG_BOOL("BoneOn",              BoneOn,              false, "骨骼瞄准开关") \
    CFG_BOOL("MemoryAimbot",        MemoryAimbot,        false, "静默自瞄开关") \
    CFG_BOOL("Missed_shot",         Missed_shot,         false, "穿透判定开关") \
    CFG_BOOL("TriggerBot",          TriggerBot,          false, "扳机功能开关") \
    CFG_BOOL("AimPrediction",       AimPrediction,       false, "预测瞄准开关") \
    CFG_BOOL("AimFovLimit",         AimFovLimit,         true,  "FOV限制开关") \
    \
    /* ===== 视觉页 ===== */ \
    CFG_BOOL("EspBox",              EspBox,              false, "显示方框") \
    CFG_BOOL("EspName",             EspName,             false, "显示名称") \
    CFG_BOOL("EspHealth",           EspHealth,           false, "显示血量") \
    CFG_BOOL("EspDistance",         EspDistance,         false, "显示距离") \
    CFG_BOOL("EspSkeleton",         EspSkeleton,         false, "显示骨骼") \
    CFG_BOOL("EspSnapline",         EspSnapline,         false, "显示射线") \
    CFG_BOOL("EspLoot",             EspLoot,             false, "显示物资") \
    CFG_BOOL("EspRadar",            EspRadar,            false, "小地图雷达") \
    CFG_BOOL("EspFillBox",          EspFillBox,          false, "填充方框") \
    CFG_BOOL("EspCornerBox",        EspCornerBox,        false, "角框模式") \
    /*CFG_BOOL("FilterZombie",        FilterZombie,        true,  "过滤僵尸") */\
    /*CFG_BOOL("FilterVampire",       FilterVampire,       true,  "过滤吸血鬼") */\
    /*CFG_BOOL("FilterTeammate",      FilterTeammate,      false, "过滤队友") */\
    /*CFG_BOOL("FilterVisibleOnly",   FilterVisibleOnly,   false, "仅可见目标") */\
    CFG_BOOL("FilterDead",          FilterDead,          true,  "过滤死亡角色") \
    \
    /* ===== 颜色页 ===== */ \
    /*CFG_BOOL("DynamicGradient",     DynamicGradient,     false, "动态渐变开关") */\
    \
    /* ===== 内存功能页 ===== */ \
    /*CFG_BOOL("PreviewNoRecoil",     PreviewNoRecoil,     false, "无后坐力预览") */\
    /*CFG_BOOL("PreviewRapidFire",    PreviewRapidFire,    false, "快速射击预览") */\
    /*CFG_BOOL("PreviewNoSpread",     PreviewNoSpread,     false, "无扩散预览") */\
    /*CFG_BOOL("PreviewSpeedHack",    PreviewSpeedHack,    false, "加速预览") */\
    /*CFG_BOOL("PreviewInfiniteAmmo", PreviewInfiniteAmmo, false, "无限弹药预览") */\
    CFG_BOOL("PreviewWorldObjects", PreviewWorldObjects, false, "显示世界对象") \
    CFG_BOOL("PreviewEntityInfo",   PreviewEntityInfo,   false, "显示实体信息") \
    CFG_BOOL("DrawObjectPointers",  DrawObjectPointers,  false, "绘制对象指针位置") \
    \
    /* ===== 设置页 ===== */ \
    CFG_BOOL("ShowTooltips",        ShowTooltips,        true,  "显示工具提示") \
    CFG_BOOL("ShowFps",             ShowFps,             true,  "显示FPS") \
    CFG_BOOL("ShowGameFps",         ShowGameFps,         true,  "显示游戏FPS") \
    CFG_BOOL("ShowOverlayFps",      ShowOverlayFps,      true,  "显示覆盖层FPS") \
    CFG_BOOL("Vsync",               Vsync,               false, "垂直同步") \
    CFG_BOOL("PerformanceMode",     PerformanceMode,     true,  "性能模式") \
    CFG_BOOL("StreamProof",         StreamProof,         false, "流媒体安全模式") \
    CFG_BOOL("PanicKeyEnabled",     PanicKeyEnabled,     true,  "紧急键启用") \
    CFG_BOOL("AutoSavePreview",     AutoSavePreview,     false, "自动保存") \
    CFG_BOOL("AutoLoadPreview",     AutoLoadPreview,     false, "启动自动加载")
    // ↑ 在此处添加新的布尔开关 ↑

// ---- 浮点型数值 ----
// 格式：CFG_FLOAT("JSON键名", 结构体字段名, 默认值, "描述")
#define CFG_FLOAT_ITEMS \
    /* ===== 自瞄页 ===== */ \
    CFG_FLOAT("Aim_Range",          Aim_Range,          120.0f, "自瞄范围") \
    CFG_FLOAT("threshold",          threshold,          8.0f,   "自瞄阈值") \
    CFG_FLOAT("AimFov",             AimFov,             35.0f,  "自瞄FOV角度") \
    CFG_FLOAT("AimSmooth",          AimSmooth,          5.0f,   "自瞄平滑度") \
    \
    /* ===== 视觉页 ===== */ \
    CFG_FLOAT("EspThickness",       EspThickness,       1.5f,   "方框线宽") \
    CFG_FLOAT("EspMaxDistance",     EspMaxDistance,     250.0f, "ESP最大距离") \
    CFG_FLOAT("EspTextScale",       EspTextScale,       1.0f,   "文字缩放") \
    \
    /* ===== 颜色页 ===== */ \
    /*CFG_FLOAT("AccentHue",          AccentHue,          0.06f,  "主题色调(H)") */\
    /*CFG_FLOAT("GlobalAlpha",        GlobalAlpha,        0.95f,  "全局透明度") */\
    \
    /* ===== 内存功能页 ===== */ \
    /*CFG_FLOAT("PreviewSpeedScale",  PreviewSpeedScale,  1.2f,   "加速倍率") */\
    \
    /* ===== 设置页 ===== */ \
    CFG_FLOAT("MenuAlpha",          MenuAlpha,          0.95f,  "菜单透明度") \
    /*CFG_FLOAT("MaxFps",             MaxFps,             144.0f, "最大帧率") */\
    CFG_FLOAT("UiScale",            UiScale,            1.0f,   "UI缩放") \
    // ↑ 在此处添加新的浮点参数 ↑

// ---- 整型数值 ----
// 格式：CFG_INT("JSON键名", 结构体字段名, 默认值, "描述")
#define CFG_INT_ITEMS \
    /* ===== 自瞄页 ===== */ \
    CFG_INT("AimBoneIndex",         AimBoneIndex,       0,      "瞄准骨骼(0头1胸2骨盆)") \
    \
    /* ===== 内存功能页 ===== */ \
    /*CFG_INT("PreviewAmmoValue",     PreviewAmmoValue,   999,    "弹药数值预览") */\
    \
    /* ===== 设置页 ===== */ \
    CFG_INT("ThemeIndex",           ThemeIndex,         0,      "主题索引(0深色1浅色)")
    // ↑ 在此处添加新的整型参数 ↑

// ---- 颜色数组 (float[4] RGBA) ----
// 格式：CFG_COLOR("JSON键名", 结构体字段名, r, g, b, a, "描述")
// 注意：颜色值必须用 4 个独立的 float 参数，不能使用 {r,g,b,a} 花括号语法，
//       因为花括号内的逗号会被预处理器误解为宏参数分隔符。
#define CFG_COLOR_ITEMS \
    CFG_COLOR("EspColor",          EspColor,          1.0f,  0.55f, 0.30f, 1.0f, "ESP主色调") \
    CFG_COLOR("ColorBox2D",        ColorBox2D,        0.30f, 0.70f, 1.00f, 1.00f, "2D方框颜色") \
    CFG_COLOR("ColorBox3D",        ColorBox3D,        0.20f, 0.60f, 0.90f, 1.00f, "3D方框颜色") \
    CFG_COLOR("ColorName",         ColorName,         1.00f, 0.80f, 0.00f, 1.00f, "名字颜色") \
    CFG_COLOR("ColorSnapline",     ColorSnapline,     0.00f, 1.00f, 0.00f, 1.00f, "射线颜色") \
    CFG_COLOR("ColorDistance",     ColorDistance,     0.80f, 0.20f, 0.80f, 1.00f, "距离颜色") \
    CFG_COLOR("ColorHealth",       ColorHealth,       0.24f, 0.92f, 0.33f, 1.00f, "血量颜色") \
    CFG_COLOR("ColorBoneVisible",  ColorBoneVisible,  0.50f, 1.00f, 0.50f, 1.00f, "可见骨骼颜色") \
    CFG_COLOR("ColorBoneOccluded", ColorBoneOccluded, 0.50f, 0.00f, 0.50f, 1.00f, "遮挡骨骼颜色") \
    CFG_COLOR("ColorRadar",        ColorRadar,        0.30f, 0.80f, 1.00f, 1.00f, "雷达颜色") \
    CFG_COLOR("ColorLoot",         ColorLoot,         1.00f, 0.65f, 0.00f, 1.00f, "物品高亮颜色")
    // ↑ 在此处添加新的颜色配置 ↑

// ============================================================================
// 由宏展开为实际的条目数组（供 Config.cpp 遍历）
// ============================================================================
// 工作原理：
//   1. 先用 #define 将 CFG_BOOL 映射为 CfgBoolEntry 初始化器
//   2. 展开 CFG_BOOL_ITEMS → 自动生成所有条目
//   3. 末尾加 {nullptr} 哨兵标记表结束
//   4. 最后 #undef 清理，避免污染其他代码
//   同理处理 CFG_FLOAT / CFG_INT / CFG_COLOR。

// ---- 布尔型表 ----
#define CFG_BOOL(key, field, defaultVal, desc) { key, &MenuSwitches::ToggleState::field, defaultVal },
static const CfgBoolEntry g_cfgBoolTable[] = {
    CFG_BOOL_ITEMS
    { nullptr, nullptr, false }  // 哨兵：标记表结束
};
#undef CFG_BOOL

// ---- 浮点型表 ----
#define CFG_FLOAT(key, field, defaultVal, desc) { key, &MenuSwitches::ToggleState::field, defaultVal },
static const CfgFloatEntry g_cfgFloatTable[] = {
    CFG_FLOAT_ITEMS
    { nullptr, nullptr, 0.0f }
};
#undef CFG_FLOAT

// ---- 整型表 ----
#define CFG_INT(key, field, defaultVal, desc) { key, &MenuSwitches::ToggleState::field, defaultVal },
static const CfgIntEntry g_cfgIntTable[] = {
    CFG_INT_ITEMS
    { nullptr, nullptr, 0 }
};
#undef CFG_INT

// ---- 颜色表 ----
// CFG_COLOR 接收 7 个参数：key, field, r, g, b, a, desc
#define CFG_COLOR(key, field, r, g, b, a, desc) { key, &MenuSwitches::ToggleState::field, { r, g, b, a } },
static const CfgColorEntry g_cfgColorTable[] = {
    CFG_COLOR_ITEMS
    { nullptr, nullptr, {0, 0, 0, 0} }
};
#undef CFG_COLOR
