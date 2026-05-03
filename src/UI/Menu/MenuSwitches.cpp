// ============================================================================
// 文件：src/UI/Menu/MenuSwitches.cpp
// 模块：UI 层 - 菜单全局状态实现
// ============================================================================

#include "MenuSwitches.h"

namespace MenuSwitches
{
    // 全局状态实例，程序生命周期内保持有效。
    ToggleState g_Toggles{};

    void ResetDefaults()
    {
        // 使用聚合初始化恢复到结构体默认值，
        // 确保新增字段后也能自动纳入默认恢复流程。
        g_Toggles = ToggleState{};
    }
}
