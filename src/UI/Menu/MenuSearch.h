// ============================================================================
// 文件：src/UI/Menu/MenuSearch.h
// 模块：UI 层 - 菜单搜索系统
// ============================================================================
// 【用途】
//   提供通用的菜单功能搜索能力，支持：
//   - 中文/英文关键词搜索
//   - 模糊匹配
//   - 搜索结果高亮
//   - 自动跳转到对应 Tab 页
//
// 【使用方法】
//   1. 在搜索框中输入关键词
//   2. 系统自动匹配所有功能项
//   3. 点击搜索结果跳转到对应功能
//
// ============================================================================

#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

namespace MenuSearch
{
    // ========================================================================
    // SearchableItem —— 可搜索的菜单项
    // ========================================================================
    struct SearchableItem
    {
        const char* displayName;    // 显示名称（中文）
        const char* englishName;    // 英文名称
        const char* keywords;       // 关键词（用空格分隔，支持多个）
        const char* description;    // 功能描述
        int         tabIndex;       // 所属 Tab 页索引（0=瞄准, 1=视觉, 2=颜色, 3=内存, 4=设置, 5=配置）
        const char* category;       // 分类（用于分组显示）
    };

    // ========================================================================
    // 全局搜索项数据库
    // ========================================================================
    // 所有可搜索的功能项都在这里注册
    static const SearchableItem g_searchDatabase[] = {
        // ===== 瞄准页 (Tab 0) =====
        { "启用自瞄",           "Aimbot",           "自瞄 瞄准 aimbot aim",                    "自动瞄准敌人",                 0, "瞄准" },
        { "骨骼瞄准",           "Bone Aim",         "骨骼 头部 bone head",                     "瞄准敌人骨骼位置",             0, "瞄准" },
        { "静默自瞄模式",       "Silent Aim",       "静默 隐藏 silent",                        "不显示准星移动",               0, "瞄准" },
        { "穿透自瞄模式",       "Wallbang Aim",     "穿透 穿墙 wallbang wall",                 "无视障碍物瞄准",               0, "瞄准" },
        { "扳机开火",           "Triggerbot",       "扳机 自动开火 trigger auto fire",         "准星对准时自动开枪",           0, "瞄准" },
        { "预测瞄准",           "Aim Prediction",   "预测 提前 prediction",                    "预判移动目标位置",             0, "瞄准" },
        { "FOV限制",            "FOV Limit",        "fov 角度 视野 field",                     "限制自瞄角度范围",             0, "瞄准" },
        { "自瞄范围",           "Aim Range",        "范围 距离 range distance",                "自瞄最大作用距离",             0, "瞄准" },
        { "平滑系数",           "Aim Smooth",       "平滑 smooth 自然",                        "自瞄移动平滑度",               0, "瞄准" },

        // ===== 视觉页 (Tab 1) =====
        { "方框 ESP",           "Box ESP",          "方框 框 box esp",                         "显示目标方框",                 1, "视觉" },
        { "名字 ESP",           "Name ESP",         "名字 名称 name",                          "显示目标名称",                 1, "视觉" },
        { "血量 ESP",           "Health ESP",       "血量 血条 health hp",                     "显示目标血量",                 1, "视觉" },
        { "距离 ESP",           "Distance ESP",     "距离 distance",                           "显示与目标距离",               1, "视觉" },
        { "骨骼绘制",           "Skeleton",         "骨骼 skeleton bone",                      "绘制目标骨骼",                 1, "视觉" },
        { "射线绘制",           "Snapline",         "射线 拉线 snapline line",                 "绘制追踪线",                   1, "视觉" },
        { "物资显示",           "Loot ESP",         "物资 物品 loot item",                     "显示可拾取物品",               1, "视觉" },
        { "雷达显示",           "Radar",            "雷达 小地图 radar minimap",               "小地图雷达",                   1, "视觉" },
        { "实心方框",           "Filled Box",       "实心 填充 filled",                        "方框内部填充",                 1, "视觉" },
        { "拐角方框",           "Corner Box",       "拐角 角 corner",                          "只显示四个角",                 1, "视觉" },
        { "过滤僵尸",           "Filter Zombie",    "僵尸 zombie filter",                      "显示僵尸目标",                 1, "视觉" },
        { "过滤吸血鬼",         "Filter Vampire",   "吸血鬼 vampire filter",                   "显示吸血鬼目标",               1, "视觉" },
        { "忽略队友",           "Ignore Teammate",  "队友 teammate team",                      "不显示队友ESP",                1, "视觉" },
        { "仅可见目标",         "Visible Only",     "可见 visible",                            "只显示可见目标",               1, "视觉" },

        // ===== 颜色页 (Tab 2) =====
        { "2D方框颜色",         "Box 2D Color",     "方框 颜色 color box 2d",                  "2D方框颜色设置",               2, "颜色" },
        { "3D方框颜色",         "Box 3D Color",     "方框 颜色 color box 3d",                  "3D方框颜色设置",               2, "颜色" },
        { "名字颜色",           "Name Color",       "名字 颜色 color name",                    "名字显示颜色",                 2, "颜色" },
        { "射线颜色",           "Snapline Color",   "射线 颜色 color snapline",                "射线显示颜色",                 2, "颜色" },
        { "距离颜色",           "Distance Color",   "距离 颜色 color distance",                "距离显示颜色",                 2, "颜色" },
        { "血量颜色",           "Health Color",     "血量 颜色 color health",                  "血量条颜色",                   2, "颜色" },
        { "可见骨骼颜色",       "Bone Visible",     "骨骼 颜色 color bone visible",            "可见骨骼颜色",                 2, "颜色" },
        { "遮挡骨骼颜色",       "Bone Occluded",    "骨骼 颜色 color bone occluded",           "遮挡骨骼颜色",                 2, "颜色" },
        { "主题色相",           "Accent Hue",       "主题 色相 hue theme",                     "主题色调设置",                 2, "颜色" },
        { "全局透明度",         "Global Alpha",     "透明 alpha opacity",                      "全局透明度",                   2, "颜色" },

        // ===== 内存页 (Tab 3) =====
        { "无后坐力",           "No Recoil",        "后坐力 recoil",                           "消除武器后坐力",               3, "内存" },
        { "快速射击",           "Rapid Fire",       "快速 射速 rapid fire",                    "提升射击速度",                 3, "内存" },
        { "无散布",             "No Spread",        "散布 精准 spread accuracy",               "消除子弹散布",                 3, "内存" },
        { "移动加速",           "Speed Hack",       "加速 速度 speed hack",                    "提升移动速度",                 3, "内存" },
        { "无限弹药",           "Infinite Ammo",    "弹药 无限 ammo infinite",                 "弹药不减少",                   3, "内存" },

        // ===== 设置页 (Tab 4) =====
        { "显示功能提示",       "Show Tooltips",    "提示 tooltip hint",                       "显示功能说明",                 4, "设置" },
        { "显示 FPS 计数器",    "Show FPS",         "fps 帧率 framerate",                      "显示帧率信息",                 4, "设置" },
        { "显示游戏 FPS",       "Show Game FPS",    "游戏 fps game",                           "显示游戏帧率",                 4, "设置" },
        { "显示覆盖层 FPS",     "Show Overlay FPS", "覆盖层 fps overlay",                      "显示覆盖层帧率",               4, "设置" },
        { "菜单透明度",         "Menu Alpha",       "透明 alpha menu",                         "菜单窗口透明度",               4, "设置" },
        { "主题选择",           "Theme",            "主题 theme 深色 浅色 dark light",         "切换菜单主题",                 4, "设置" },
        { "UI缩放比例",         "UI Scale",         "缩放 scale ui 大小 size",                 "调整UI大小",                   4, "设置" },
        { "最大 FPS",           "Max FPS",          "帧率 fps 限制 limit",                     "帧率上限",                     4, "设置" },
        { "垂直同步",           "VSync",            "垂直 同步 vsync",                         "垂直同步开关",                 4, "设置" },
        { "性能模式",           "Performance Mode", "性能 performance 优化",                   "关闭特效提升性能",             4, "设置" },
        { "流媒体隐藏",         "Stream Proof",     "流媒体 直播 stream obs",                  "直播时隐藏菜单",               4, "设置" },
        { "启用紧急键",         "Panic Key",        "紧急 panic 快捷键",                       "紧急关闭快捷键",               4, "设置" },

        // ===== 配置页 (Tab 5) =====
        { "保存配置",           "Save Config",      "保存 save config",                        "保存当前设置",                 5, "配置" },
        { "读取配置",           "Load Config",      "读取 加载 load config",                   "加载已保存设置",               5, "配置" },
        { "重置配置",           "Reset Config",     "重置 恢复 reset default",                 "恢复默认设置",                 5, "配置" },
        { "自动读取配置",       "Auto Load",        "自动 加载 auto load",                     "启动时自动加载",               5, "配置" },
        { "自动保存配置",       "Auto Save",        "自动 保存 auto save",                     "运行时自动保存",               5, "配置" },
    };

    static constexpr size_t g_searchDatabaseSize = sizeof(g_searchDatabase) / sizeof(SearchableItem);

    // ========================================================================
    // 搜索结果结构
    // ========================================================================
    struct SearchResult
    {
        const SearchableItem* item;
        int matchScore;  // 匹配分数，越高越相关

        bool operator>(const SearchResult& other) const
        {
            return matchScore > other.matchScore;
        }
    };

    // ========================================================================
    // 工具函数：字符串转小写（用于不区分大小写搜索）
    // ========================================================================
    inline std::string ToLower(const std::string& str)
    {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return result;
    }

    // ========================================================================
    // 工具函数：检查字符串是否包含子串（不区分大小写）
    // ========================================================================
    inline bool ContainsIgnoreCase(const std::string& haystack, const std::string& needle)
    {
        if (needle.empty()) return true;
        std::string lowerHaystack = ToLower(haystack);
        std::string lowerNeedle = ToLower(needle);
        return lowerHaystack.find(lowerNeedle) != std::string::npos;
    }

    // ========================================================================
    // 核心函数：执行搜索
    // ========================================================================
    // 参数：
    //   query - 搜索关键词
    //   results - 输出搜索结果（按相关度排序）
    //   maxResults - 最多返回多少条结果（默认 10）
    // 返回：
    //   找到的结果数量
    inline int Search(const char* query, std::vector<SearchResult>& results, int maxResults = 10)
    {
        results.clear();
        if (!query || query[0] == '\0')
            return 0;

        std::string queryStr(query);
        std::string queryLower = ToLower(queryStr);

        // 遍历所有可搜索项
        for (size_t i = 0; i < g_searchDatabaseSize; ++i)
        {
            const SearchableItem& item = g_searchDatabase[i];
            int score = 0;

            // 1. 检查显示名称（中文）- 权重最高
            if (ContainsIgnoreCase(item.displayName, queryStr))
                score += 100;

            // 2. 检查英文名称 - 权重高
            if (ContainsIgnoreCase(item.englishName, queryLower))
                score += 80;

            // 3. 检查关键词 - 权重中等
            if (ContainsIgnoreCase(item.keywords, queryLower))
                score += 50;

            // 4. 检查描述 - 权重较低
            if (ContainsIgnoreCase(item.description, queryStr))
                score += 30;

            // 5. 检查分类 - 权重最低
            if (ContainsIgnoreCase(item.category, queryStr))
                score += 10;

            // 如果有匹配，加入结果
            if (score > 0)
            {
                SearchResult result;
                result.item = &item;
                result.matchScore = score;
                results.push_back(result);
            }
        }

        // 按匹配分数降序排序
        std::sort(results.begin(), results.end(), std::greater<SearchResult>());

        // 限制结果数量
        if (static_cast<int>(results.size()) > maxResults)
            results.resize(maxResults);

        return static_cast<int>(results.size());
    }

    // ========================================================================
    // 辅助函数：获取 Tab 名称
    // ========================================================================
    inline const char* GetTabName(int tabIndex)
    {
        static const char* tabNames[] = {
            "瞄准", "视觉", "颜色", "内存", "设置", "配置"
        };
        if (tabIndex >= 0 && tabIndex < 6)
            return tabNames[tabIndex];
        return "未知";
    }

} // namespace MenuSearch
