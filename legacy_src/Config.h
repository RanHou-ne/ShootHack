#ifndef CONFIG_H
#define CONFIG_H

#include <string>

// ============================================================================
// 配置管理（自动序列化引擎）
// ============================================================================
// 【使用方式】
//   1. 在 ConfigSchema.h 中用宏声明需要保存的配置项（含中文注释）
//   2. 调用 Config::Initialize("目录路径") 初始化
//   3. Config::Save("settings.json")   → 自动保存所有声明项
//   4. Config::Load("settings.json")   → 自动读取并回填所有声明项
//   5. Config::ResetToDefaults()       → 全部恢复为默认值
//
// 【无需修改本文件】新增配置项只需编辑 ConfigSchema.h 加一行宏即可。
// ============================================================================

class Config
{
public:
    // 初始化配置系统（创建目录等）
    static void Initialize(const std::string& configPath);

    // ---------- 核心接口 ----------

    /// 保存配置：自动遍历 ConfigSchema.h 中所有声明的配置项写入 JSON
    static bool Save(const std::string& filename);

    /// 加载配置：自动读取 JSON 并回填到 g_Toggles 对应字段
    static bool Load(const std::string& filename);

    /// 重置所有配置为默认值（依赖 ConfigSchema.h 中的默认值定义）
    static void ResetToDefaults();

    // ---------- 兼容旧接口（内部委托到新接口） ----------

    static bool SaveConfig(const std::string& filename)   { return Save(filename); }
    static bool LoadConfig(const std::string& filename)   { return Load(filename); }
    static bool SaveAutoSettings(const std::string& filename, bool autoLoad, bool autoSave);
    static bool LoadAutoSettings(const std::string& filename, bool& autoLoad, bool& autoSave);

    // ---------- 工具 ----------

    static bool FileExists(const std::string& filename);
    static bool CreateConfigDirectory(const std::string& dirPath);

private:
    static std::string s_configPath;
};

#endif // CONFIG_H

