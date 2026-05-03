// ============================================================================
// 文件：src/Config/Config.h
// 模块：配置层 - 配置序列化引擎接口
// ============================================================================
// 【用途】
//   提供配置文件的保存、读取和重置功能。
//   采用 X-Macro 技术（ConfigSchema.h）自动遍历所有配置项，
//   无需手动维护序列化/反序列化代码。
//
// 【使用方式】
//   1. 在 ConfigSchema.h 中用宏声明需要保存的配置项
//   2. 调用 Config::Initialize(".\\config") 初始化目录
//   3. Config::Save("settings.json")   → 自动保存所有声明项
//   4. Config::Load("settings.json")   → 自动读取并回填所有声明项
//   5. Config::ResetToDefaults()       → 全部恢复为默认值
//
// 【无需修改本文件】
//   新增配置项只需编辑 ConfigSchema.h 加一行宏即可，
//   保存/读取/重置全部自动生效。
//
// 【扩展指南】
//   如果需要支持多配置文件（如 profiles）：
//   1. 在 Initialize() 中接受 profile 名称参数
//   2. 在 Save/Load 中拼接 profile 路径
//   3. 添加 ListProfiles() / SwitchProfile() 接口
// ============================================================================

#pragma once

#include <string>

class Config
{
public:
    // ---- 初始化 ----
    // 设置配置文件存放目录，并创建目录（如不存在）。
    // 必须在 Save/Load 之前调用一次。
    static void Initialize(const std::string& configPath);

    // ---- 核心接口 ----

    /// 保存配置：自动遍历 ConfigSchema.h 中所有声明的配置项写入 JSON
    static bool Save(const std::string& filename);

    /// 加载配置：自动读取 JSON 并回填到 g_Toggles 对应字段
    static bool Load(const std::string& filename);

    /// 重置所有配置为默认值（依赖 ConfigSchema.h 中的默认值定义）
    static void ResetToDefaults();

    // ---- 兼容旧接口（内部委托到新接口） ----
    static bool SaveConfig(const std::string& filename)   { return Save(filename); }
    static bool LoadConfig(const std::string& filename)   { return Load(filename); }

    // ---- 自动化策略文件（独立于主配置） ----
    // 保存/读取 AutoLoad / AutoSave 两个布尔标志到独立 JSON 文件
    static bool SaveAutoSettings(const std::string& filename, bool autoLoad, bool autoSave);
    static bool LoadAutoSettings(const std::string& filename, bool& autoLoad, bool& autoSave);

    // ---- 工具 ----
    static bool FileExists(const std::string& filename);
    static bool CreateConfigDirectory(const std::string& dirPath);

private:
    static std::string s_configPath;  // 配置目录路径（由 Initialize 设置）
};
