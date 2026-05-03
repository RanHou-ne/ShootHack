# FPS 计数器功能说明

## 功能概述

FPS 计数器功能已完整实现，可以在游戏右上角实时显示帧率信息。该功能支持分别显示游戏 FPS 和覆盖层 FPS，并提供可折叠的 UI 控制选项。

## 功能特性

### 1. 双 FPS 显示
- **游戏 FPS**：显示游戏本身的帧率（Present 调用频率）
- **覆盖层 FPS**：显示 ImGui 菜单覆盖层的渲染帧率

### 2. 智能颜色编码
FPS 数值根据性能自动变色：
- **绿色**：FPS ≥ 60（性能良好）
- **黄色**：30 ≤ FPS < 60（性能一般）
- **红色**：FPS < 30（性能较差）

### 3. 详细信息显示
- FPS 数值（每秒帧数）
- 帧时间（毫秒）
- 总帧数统计

### 4. 可折叠 UI 控制
在菜单的"设置"页面中，FPS 计数器提供三级控制：
```
☑ 显示 FPS 计数器          ← 主开关
    ☑ 显示游戏 FPS         ← 子选项1：控制游戏FPS显示
    ☑ 显示覆盖层 FPS       ← 子选项2：控制覆盖层FPS显示
```

## 使用方法

### 启用 FPS 计数器
1. 按 `INSERT` 键打开菜单
2. 切换到"设置"（Element Setup）页面
3. 勾选"显示 FPS 计数器"

### 自定义显示内容
当"显示 FPS 计数器"被勾选后，会出现两个子选项：
- **显示游戏 FPS**：控制是否显示游戏本身的帧率
- **显示覆盖层 FPS**：控制是否显示菜单覆盖层的帧率

你可以根据需要选择显示其中一个或两个都显示。

### 完全关闭 FPS 显示
取消勾选"显示 FPS 计数器"即可完全关闭 FPS 显示窗口。

## 技术实现

### 核心组件

#### 1. FPS 计数器类（`src/Core/FpsCounter.h`）
```cpp
class FpsCounter
{
    // 使用滑动窗口算法计算平滑的 FPS
    // 默认 60 帧窗口，避免数值跳动
};
```

#### 2. 全局 FPS 计数器实例（`src/Hook/DX11Hook.cpp`）
```cpp
static FpsCounter g_gameFpsCounter(60);      // 游戏FPS计数器
static FpsCounter g_overlayFpsCounter(60);   // 覆盖层FPS计数器
```

#### 3. UI 控制开关（`src/UI/Menu/MenuSwitches.h`）
```cpp
bool ShowFps         = true;    // FPS 计数器主开关
bool ShowGameFps     = true;    // 显示游戏 FPS
bool ShowOverlayFps  = true;    // 显示覆盖层 FPS
```

### 渲染流程

1. **更新游戏 FPS**：在每次 `Present` 调用时更新
   ```cpp
   g_gameFpsCounter.Update();
   ```

2. **更新覆盖层 FPS**：在 ImGui 渲染前更新
   ```cpp
   g_overlayFpsCounter.Update();
   ```

3. **条件渲染**：根据用户设置决定显示内容
   ```cpp
   if (MenuSwitches::g_Toggles.ShowFps)
   {
       RenderFpsDisplay();  // 根据 ShowGameFps 和 ShowOverlayFps 决定显示内容
   }
   ```

### 配置持久化

FPS 计数器的所有设置都会自动保存到配置文件中：
- `ShowFps`：主开关状态
- `ShowGameFps`：游戏 FPS 显示状态
- `ShowOverlayFps`：覆盖层 FPS 显示状态

配置文件位置：`config/settings.json`

## 性能影响

FPS 计数器本身对性能的影响极小：
- 使用高效的滑动窗口算法
- 仅在需要时渲染显示窗口
- 不进行复杂的图形绘制

## 扩展建议

如果需要进一步扩展 FPS 计数器功能，可以考虑：

1. **添加更多统计信息**
   - 最小/最大 FPS
   - 1% 低帧率（性能稳定性指标）
   - 平均帧时间

2. **自定义显示位置**
   - 添加位置选择（左上/右上/左下/右下）
   - 支持拖动调整位置

3. **性能图表**
   - 添加实时 FPS 曲线图
   - 帧时间柱状图

4. **性能预警**
   - FPS 低于阈值时发出警告
   - 帧时间波动过大时提示

## 相关文件

### 核心实现
- `src/Core/FpsCounter.h` - FPS 计数器类定义
- `src/Hook/DX11Hook.cpp` - FPS 更新和渲染逻辑
- `src/UI/Menu/Menu.cpp` - UI 控制界面
- `src/UI/Menu/MenuSwitches.h` - 开关状态定义
- `src/Config/ConfigSchema.h` - 配置项定义

### 文档
- `docs/UI实操教程.md` - UI 开发教程
- `docs/配置系统使用指南.md` - 配置系统使用指南

## 常见问题

### Q: FPS 显示窗口位置可以调整吗？
A: 当前版本固定在右上角。如需调整，可以修改 `DX11Hook.cpp` 中 `RenderFpsDisplay()` 函数的 `windowPos` 计算。

### Q: 为什么游戏 FPS 和覆盖层 FPS 不一样？
A: 这是正常现象。游戏 FPS 反映游戏引擎的渲染速度，覆盖层 FPS 反映 ImGui 菜单的渲染速度。两者可能因为渲染复杂度不同而有差异。

### Q: FPS 数值为什么会波动？
A: 虽然使用了 60 帧滑动窗口平滑算法，但仍会有轻微波动。这反映了真实的性能变化。如需更平滑的显示，可以增加滑动窗口大小。

### Q: 如何完全禁用 FPS 计数器以提升性能？
A: 在菜单设置页面取消勾选"显示 FPS 计数器"即可。禁用后不会进行任何 FPS 相关的渲染，但 FPS 统计仍会在后台进行（性能影响可忽略）。

## 更新日志

### v1.0 (2026-04-27)
- ✅ 实现基础 FPS 计数器功能
- ✅ 支持游戏 FPS 和覆盖层 FPS 分别显示
- ✅ 添加可折叠的 UI 控制选项
- ✅ 实现颜色编码（绿/黄/红）
- ✅ 集成配置系统，支持设置持久化
- ✅ 添加帧时间和总帧数显示

---

**开发者**: Kiro AI Assistant  
**最后更新**: 2026-04-27
