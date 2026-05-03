#include "MenuSwitches.h"

namespace MenuSwitches
{
    // 全局状态实例，程序生命周期内保持有效。
    // 如果后续需要“每局重置”或“配置热加载”，只需在对应流程调用 ResetDefaults/赋值。
    ToggleState g_Toggles{};

    void ResetDefaults()
    {
        // 使用聚合初始化恢复到结构体默认值，
        // 可确保新增字段后也能自动纳入默认恢复流程。
        g_Toggles = ToggleState{};
    }
}
