#pragma once

namespace MenuFeatureStubs
{
    // 预留接口层：
    // 作用是把 UI 与真实功能实现解耦。
    // 当前所有函数仅返回“是否已接入实现”，便于菜单展示功能可能性。

    // ===== Aimbot 预留接口 =====
    bool ApplyAimbotConfig(bool enable, float range, float smooth, int boneIndex);
    bool TriggerSingleShot();

    // ===== ESP 预留接口 =====
    bool ApplyEspConfig(float maxDistance, float thickness, float textScale);

    // ===== Memory 预留接口 =====
    bool ApplyMemoryPreview(float speedScale, int ammoValue);

    // ===== UI/Theme 预留接口 =====
    bool ApplyThemeConfig(float accentHue, float alpha, float uiScale);

    // 用于在菜单中显示统一状态文案。
    const char* NotImplementedMessage();
}
