// ============================================================================
// 文件：src/Config/Config.cpp
// 模块：配置层 - 配置序列化引擎实现
// ============================================================================
// 【实现原理】
//   使用 X-Macro 技术（ConfigSchema.h）自动生成配置项表，
//   Save/Load/ResetToDefaults 只需遍历这些表即可，无需手动维护。
//
// 【JSON 库】
//   使用 nlohmann/json（mofang/json.hpp），单头文件，无需额外依赖。
//
// 【扩展指南】
//   新增配置项：只需在 ConfigSchema.h 对应区域加一行宏，
//   本文件无需任何修改。
// ============================================================================

#include "Config.h"
#include "ConfigSchema.h"          // ← 配置项宏表（新增配置只需编辑此文件）
#include "UI/Menu/MenuSwitches.h"
#include "ThirdParty/json.hpp"     // nlohmann/json 第三方库
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <cstdio>

namespace
{
    using nlohmann::json;

    // ---- 路径工具 ----
    std::string BuildConfigPath(const std::string& baseDir, const std::string& filename)
    {
        if (filename.empty())
            return baseDir;
        return baseDir + "\\" + filename;
    }

    // ---- JSON 读取辅助 ----
    template <typename T>
    void ReadNumber(const json& j, const char* key, T& out)
    {
        auto it = j.find(key);
        if (it != j.end() && it->is_number())
            out = it->get<T>();
    }

    void ReadBool(const json& j, const char* key, bool& out)
    {
        auto it = j.find(key);
        if (it != j.end() && it->is_boolean())
            out = it->get<bool>();
    }

    void ReadFloat4(const json& j, const char* key, float out[4])
    {
        auto it = j.find(key);
        if (it == j.end() || !it->is_array() || it->size() < 4)
            return;
        for (int i = 0; i < 4; ++i)
            if ((*it)[i].is_number())
                out[i] = (*it)[i].get<float>();
    }
}

std::string Config::s_configPath = ".\\config";

// ============================================================================
// Initialize() —— 初始化配置目录
// ============================================================================

void Config::Initialize(const std::string& configPath)
{
    s_configPath = configPath;
    CreateConfigDirectory(s_configPath);
}

// ============================================================================
// Save() —— 保存配置
// ============================================================================
// 自动遍历 ConfigSchema.h 中的宏表，将所有配置项写入 JSON 文件。
// 新增配置项只需在 ConfigSchema.h 加一行宏，此处无需修改！

bool Config::Save(const std::string& filename)
{
    const std::string fullPath = BuildConfigPath(s_configPath, filename);
    printf("[Config] 正在保存: %s\n", fullPath.c_str());

    if (!CreateConfigDirectory(s_configPath))
        return false;

    const auto& t = MenuSwitches::g_Toggles;
    json j;

    // 1) 布尔型开关
    for (int i = 0; ; ++i)
    {
        const CfgBoolEntry& e = g_cfgBoolTable[i];
        if (e.key == nullptr) break;
        j[e.key] = t.*(e.fieldPtr);
    }

    // 2) 浮点型数值
    for (int i = 0; ; ++i)
    {
        const CfgFloatEntry& e = g_cfgFloatTable[i];
        if (e.key == nullptr) break;
        j[e.key] = t.*(e.fieldPtr);
    }

    // 3) 整型数值
    for (int i = 0; ; ++i)
    {
        const CfgIntEntry& e = g_cfgIntTable[i];
        if (e.key == nullptr) break;
        j[e.key] = t.*(e.fieldPtr);
    }

    // 4) RGBA 颜色数组
    for (int i = 0; ; ++i)
    {
        const CfgColorEntry& e = g_cfgColorTable[i];
        if (e.key == nullptr) break;
        const float* c = t.*(e.fieldPtr);
        j[e.key] = { c[0], c[1], c[2], c[3] };
    }

    try
    {
        std::ofstream file(fullPath, std::ios::out | std::ios::trunc);
        if (!file)
        {
            printf("[Config] 保存失败：无法打开文件 %s\n", fullPath.c_str());
            return false;
        }
        file << j.dump(4);
        file.close();
    }
    catch (const std::exception& ex)
    {
        printf("[Config] 保存异常: %s\n", ex.what());
        return false;
    }

    printf("[Config] 保存成功: %s\n", fullPath.c_str());
    return true;
}

// ============================================================================
// Load() —— 加载配置
// ============================================================================
// 自动遍历 ConfigSchema.h 中的宏表，从 JSON 文件回填所有配置项。
// 新增配置项只需在 ConfigSchema.h 加一行宏，此处无需修改！

bool Config::Load(const std::string& filename)
{
    const std::string fullPath = BuildConfigPath(s_configPath, filename);
    printf("[Config] 正在加载: %s\n", fullPath.c_str());

    if (!FileExists(fullPath))
    {
        printf("[Config] 文件不存在: %s\n", fullPath.c_str());
        return false;
    }

    try
    {
        std::ifstream file(fullPath);
        if (!file)
        {
            printf("[Config] 加载失败：无法打开文件 %s\n", fullPath.c_str());
            return false;
        }

        json j;
        file >> j;
        file.close();

        auto& t = MenuSwitches::g_Toggles;

        // 1) 布尔型开关
        for (int i = 0; ; ++i)
        {
            const CfgBoolEntry& e = g_cfgBoolTable[i];
            if (e.key == nullptr) break;
            ReadBool(j, e.key, t.*(e.fieldPtr));
        }

        // 2) 浮点型数值
        for (int i = 0; ; ++i)
        {
            const CfgFloatEntry& e = g_cfgFloatTable[i];
            if (e.key == nullptr) break;
            ReadNumber(j, e.key, t.*(e.fieldPtr));
        }

        // 3) 整型数值
        for (int i = 0; ; ++i)
        {
            const CfgIntEntry& e = g_cfgIntTable[i];
            if (e.key == nullptr) break;
            ReadNumber(j, e.key, t.*(e.fieldPtr));
        }

        // 4) RGBA 颜色数组
        for (int i = 0; ; ++i)
        {
            const CfgColorEntry& e = g_cfgColorTable[i];
            if (e.key == nullptr) break;
            ReadFloat4(j, e.key, t.*(e.fieldPtr));
        }
    }
    catch (const std::exception& ex)
    {
        printf("[Config] 加载异常: %s\n", ex.what());
        return false;
    }

    printf("[Config] 加载成功: %s\n", fullPath.c_str());
    return true;
}

// ============================================================================
// ResetToDefaults() —— 重置所有配置为默认值
// ============================================================================
// 自动遍历 ConfigSchema.h 中的宏表，将所有字段恢复为声明时的默认值。

void Config::ResetToDefaults()
{
    auto& t = MenuSwitches::g_Toggles;

    for (int i = 0; ; ++i)
    {
        const CfgBoolEntry& e = g_cfgBoolTable[i];
        if (e.key == nullptr) break;
        t.*(e.fieldPtr) = e.defaultValue;
    }

    for (int i = 0; ; ++i)
    {
        const CfgFloatEntry& e = g_cfgFloatTable[i];
        if (e.key == nullptr) break;
        t.*(e.fieldPtr) = e.defaultValue;
    }

    for (int i = 0; ; ++i)
    {
        const CfgIntEntry& e = g_cfgIntTable[i];
        if (e.key == nullptr) break;
        t.*(e.fieldPtr) = e.defaultValue;
    }

    for (int i = 0; ; ++i)
    {
        const CfgColorEntry& e = g_cfgColorTable[i];
        if (e.key == nullptr) break;
        float* c = t.*(e.fieldPtr);
        for (int k = 0; k < 4; ++k)
            c[k] = e.defaultValue[k];
    }

    printf("[Config] 已重置为默认值\n");
}

// ============================================================================
// SaveAutoSettings / LoadAutoSettings —— 自动化策略文件
// ============================================================================

bool Config::SaveAutoSettings(const std::string& filename, bool autoLoad, bool autoSave)
{
    if (!CreateConfigDirectory(s_configPath))
        return false;

    const std::string fullPath = BuildConfigPath(s_configPath, filename);
    nlohmann::json j;
    j["AutoLoad"] = autoLoad;
    j["AutoSave"] = autoSave;

    try
    {
        std::ofstream file(fullPath, std::ios::out | std::ios::trunc);
        if (!file) return false;
        file << j.dump(4);
    }
    catch (const std::exception& ex)
    {
        printf("[Config] SaveAutoSettings 异常: %s\n", ex.what());
        return false;
    }
    return true;
}

bool Config::LoadAutoSettings(const std::string& filename, bool& autoLoad, bool& autoSave)
{
    const std::string fullPath = BuildConfigPath(s_configPath, filename);
    if (!FileExists(fullPath))
        return false;

    try
    {
        std::ifstream file(fullPath);
        if (!file) return false;
        nlohmann::json j;
        file >> j;
        ReadBool(j, "AutoLoad", autoLoad);
        ReadBool(j, "AutoSave", autoSave);
    }
    catch (const std::exception& ex)
    {
        printf("[Config] LoadAutoSettings 异常: %s\n", ex.what());
        return false;
    }
    return true;
}

// ============================================================================
// 工具函数
// ============================================================================

bool Config::FileExists(const std::string& filename)
{
    DWORD attrs = GetFileAttributesA(filename.c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES) && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

bool Config::CreateConfigDirectory(const std::string& dirPath)
{
    if (std::filesystem::exists(dirPath))
        return true;

    try
    {
        std::filesystem::create_directories(dirPath);
        printf("[Config] 目录已创建: %s\n", dirPath.c_str());
        return true;
    }
    catch (const std::exception& ex)
    {
        printf("[Config] 创建目录失败: %s\n", ex.what());
        return false;
    }
}
