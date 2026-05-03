#include "Config.h"
#include "ConfigSchema.h"                 // ← 配置项宏表（新增配置只需编辑此文件）
#include "Drawing/MenuSwitches.h"
#include "mofang/json.hpp"
#include <windows.h>
#include <direct.h>
#include <cstdio>
#include <fstream>

namespace
{
    using nlohmann::json;

    // ---------- 路径工具 ----------

    std::string BuildConfigPath(const std::string& baseDir, const std::string& filename)
    {
        if (filename.empty())
            return baseDir;
        return baseDir + "\\" + filename;
    }

    // ---------- JSON 读取辅助（用于 Load 流程） ----------

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
        {
            if ((*it)[i].is_number())
                out[i] = (*it)[i].get<float>();
        }
    }
}

std::string Config::s_configPath = ".\\config";

// ============================================================================
// 初始化
// ============================================================================

void Config::Initialize(const std::string& configPath)
{
    s_configPath = configPath;
    CreateConfigDirectory(s_configPath);
}

// ============================================================================
// 保存配置（自动遍历 ConfigSchema.h 中的宏表）
// ============================================================================

bool Config::Save(const std::string& filename)
{
    const std::string fullPath = BuildConfigPath(s_configPath, filename);
    printf("[*] 正在保存配置: %s\n", fullPath.c_str());

    if (!CreateConfigDirectory(s_configPath))
        return false;

    const auto& t = MenuSwitches::g_Toggles;
    json j;

    // ---- 遍历所有宏表条目，自动写入 JSON ----
    // 新增配置项只需在 ConfigSchema.h 加一行宏，此处无需修改！

    // 1) 布尔型开关
    for (int i = 0; ; ++i)
    {
        const CfgBoolEntry& entry = g_cfgBoolTable[i];
        if (entry.key == nullptr) break;          // 哨兵，表结束
        j[entry.key] = t.*(entry.fieldPtr);
    }

    // 2) 浮点型数值
    for (int i = 0; ; ++i)
    {
        const CfgFloatEntry& entry = g_cfgFloatTable[i];
        if (entry.key == nullptr) break;
        j[entry.key] = t.*(entry.fieldPtr);
    }

    // 3) 整型数值
    for (int i = 0; ; ++i)
    {
        const CfgIntEntry& entry = g_cfgIntTable[i];
        if (entry.key == nullptr) break;
        j[entry.key] = t.*(entry.fieldPtr);
    }

    // 4) RGBA 颜色数组
    for (int i = 0; ; ++i)
    {
        const CfgColorEntry& entry = g_cfgColorTable[i];
        if (entry.key == nullptr) break;
        const float* color = t.*(entry.fieldPtr);
        j[entry.key] = { color[0], color[1], color[2], color[3] };
    }

    // 写入文件
    try
    {
        std::ofstream file(fullPath, std::ios::out | std::ios::trunc);
        if (!file)
        {
            printf("[-] 保存失败：无法打开文件 %s\n", fullPath.c_str());
            return false;
        }
        file << j.dump(4);
        file.close();
    }
    catch (const std::exception& ex)
    {
        printf("[-] 保存失败: %s\n", ex.what());
        return false;
    }

    printf("[+] 配置已保存: %s\n", fullPath.c_str());
    return true;
}

// ============================================================================
// 加载配置（自动遍历 ConfigSchema.h 中的宏表）
// ============================================================================

bool Config::Load(const std::string& filename)
{
    const std::string fullPath = BuildConfigPath(s_configPath, filename);
    printf("[*] 正在加载配置: %s\n", fullPath.c_str());

    if (!FileExists(fullPath))
    {
        printf("[-] 配置文件不存在: %s\n", fullPath.c_str());
        return false;
    }

    try
    {
        std::ifstream file(fullPath);
        if (!file)
        {
            printf("[-] 加载失败：无法打开文件 %s\n", fullPath.c_str());
            return false;
        }

        json j;
        file >> j;
        file.close();

        auto& t = MenuSwitches::g_Toggles;

        // ---- 遍历所有宏表条目，自动从 JSON 回填 ----
        // 新增配置项只需在 ConfigSchema.h 加一行宏，此处无需修改！

        // 1) 布尔型开关
        for (int i = 0; ; ++i)
        {
            const CfgBoolEntry& entry = g_cfgBoolTable[i];
            if (entry.key == nullptr) break;
            ReadBool(j, entry.key, t.*(entry.fieldPtr));
        }

        // 2) 浮点型数值
        for (int i = 0; ; ++i)
        {
            const CfgFloatEntry& entry = g_cfgFloatTable[i];
            if (entry.key == nullptr) break;
            ReadNumber(j, entry.key, t.*(entry.fieldPtr));
        }

        // 3) 整型数值
        for (int i = 0; ; ++i)
        {
            const CfgIntEntry& entry = g_cfgIntTable[i];
            if (entry.key == nullptr) break;
            ReadNumber(j, entry.key, t.*(entry.fieldPtr));
        }

        // 4) RGBA 颜色数组
        for (int i = 0; ; ++i)
        {
            const CfgColorEntry& entry = g_cfgColorTable[i];
            if (entry.key == nullptr) break;
            ReadFloat4(j, entry.key, t.*(entry.fieldPtr));
        }
    }
    catch (const std::exception& ex)
    {
        printf("[-] 加载失败: %s\n", ex.what());
        return false;
    }

    printf("[+] 配置已加载: %s\n", fullPath.c_str());
    return true;
}

// ============================================================================
// 重置所有配置为默认值（自动遍历 ConfigSchema.h 中的宏表）
// ============================================================================

void Config::ResetToDefaults()
{
    auto& t = MenuSwitches::g_Toggles;

    // 1) 布尔型 → 恢复默认值
    for (int i = 0; ; ++i)
    {
        const CfgBoolEntry& entry = g_cfgBoolTable[i];
        if (entry.key == nullptr) break;
        t.*(entry.fieldPtr) = entry.defaultValue;
    }

    // 2) 浮点型 → 恢复默认值
    for (int i = 0; ; ++i)
    {
        const CfgFloatEntry& entry = g_cfgFloatTable[i];
        if (entry.key == nullptr) break;
        t.*(entry.fieldPtr) = entry.defaultValue;
    }

    // 3) 整型 → 恢复默认值
    for (int i = 0; ; ++i)
    {
        const CfgIntEntry& entry = g_cfgIntTable[i];
        if (entry.key == nullptr) break;
        t.*(entry.fieldPtr) = entry.defaultValue;
    }

    // 4) 颜色 → 恢复默认值
    for (int i = 0; ; ++i)
    {
        const CfgColorEntry& entry = g_cfgColorTable[i];
        if (entry.key == nullptr) break;
        float* color = t.*(entry.fieldPtr);
        for (int c = 0; c < 4; ++c)
            color[c] = entry.defaultValue[c];
    }

    printf("[+] 配置已重置为默认值\n");
}

// ============================================================================
// 自动配置（AutoSave / AutoLoad）独立文件
// ============================================================================

bool Config::SaveAutoSettings(const std::string& filename, bool autoLoad, bool autoSave)
{
    if (!CreateConfigDirectory(s_configPath))
        return false;

    const std::string fullPath = BuildConfigPath(s_configPath, filename);
    json j;
    j["AutoLoad"] = autoLoad;
    j["AutoSave"] = autoSave;

    try
    {
        std::ofstream file(fullPath, std::ios::out | std::ios::trunc);
        if (!file)
            return false;
        file << j.dump(4);
    }
    catch (const std::exception& ex)
    {
        printf("[-] Save auto settings failed: %s\n", ex.what());
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
        if (!file)
            return false;

        json j;
        file >> j;

        ReadBool(j, "AutoLoad", autoLoad);
        ReadBool(j, "AutoSave", autoSave);
    }
    catch (const std::exception& ex)
    {
        printf("[-] Load auto settings failed: %s\n", ex.what());
        return false;
    }

    return true;
}

bool Config::FileExists(const std::string& filename)
{
    DWORD attrs = GetFileAttributesA(filename.c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES) && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

bool Config::CreateConfigDirectory(const std::string& dirPath)
{
    if (std::filesystem::exists(dirPath))
    {
        return true;
    }

    try
    {
        std::filesystem::create_directories(dirPath);
        printf("[+] Config directory created: %s\n", dirPath.c_str());
        return true;
    }
    catch (const std::exception& ex)
    {
        printf("[-] Failed to create config directory: %s\n", ex.what());
        return false;
    }
}
