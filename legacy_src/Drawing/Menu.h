#pragma once

#include "imgui/imgui.h"
#include "globals.h"

// ==================== 菜单命名空间 ====================

namespace Menu
{
    // ========== 菜单状态 ==========
    // 菜单是否打开。由 Insert 键切换（见 DX11.cpp）。
    // true: 渲染完整菜单并接管部分输入。
    // false: 不绘制菜单。
    extern bool IsMenuOpen;

    // ========== 主渲染函数 ==========
    // 每帧调用一次。内部负责：
    // 1) 背景和主框体绘制
    // 2) 侧边 Tab 切换与动画
    // 3) 调用各子面板绘制函数
    void Render();

    // 菜单初始化（资源就绪后调用一次）。
    void Initialize();
    // 菜单关闭/卸载时调用，释放菜单层面资源（当前主要用于日志）。
    void Shutdown();

    // ========== Tab 页面 ==========
    // 以下函数只负责“某个页面内容”的绘制。
    // 建议修改规则：
    // - 新增控件：先在 MenuSwitches::ToggleState 加字段，再在这里绑定。
    // - 复杂逻辑：放到独立模块，避免堆在 UI 文件里。
    void DrawOverviewTab();
    void DrawAimbotTab();
    void DrawESPTab();
    void DrawSettingsTab();
    void DrawConfigTab();

    // ========== UI 组件 ==========
    // 预留扩展：如后续拆分头部/底部区域，可在这些函数实现。
    void DrawMenuHeader();
    void DrawTabButtons();
    void DrawFooter();

    // ========== 工具函数 ==========
    // 切换菜单开关。一般由键盘热键触发。
    void ToggleMenu();

} // namespace Menu
