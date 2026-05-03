# ShootHack 魔改 ImGui 完整使用指南

> **基于版本**: ImGui 1.89.6 (`IMGUI_VERSION_NUM 18960`)
> **渲染后端**: DirectX 11 (`imgui_impl_dx11`)
> **平台后端**: Win32 (`imgui_impl_win32`)
> **FreeType**: 已包含源码但未启用（`IMGUI_ENABLE_FREETYPE` 未定义）
> **文档生成日期**: 2026-04-30

---

## 目录

- [1. 魔改概述](#1-魔改概述)
- [2. 主题与样式系统 (imgui_settings.h)](#2-主题与样式系统)
- [3. 自定义控件详解](#3-自定义控件详解)
  - [3.1 Tab —— 自定义标签页按钮](#31-tab--自定义标签页按钮)
  - [3.2 SubTab —— 子标签页图片按钮](#32-subtab--子标签页图片按钮)
  - [3.3 CustomButton —— 自定义图片按钮](#33-custombutton--自定义图片按钮)
  - [3.4 Checkbox —— 重写开关样式](#34-checkbox--重写开关样式)
  - [3.5 CheckPicker —— 复选框+颜色拾取器组合](#35-checkpicker--复选框颜色拾取器组合)
  - [3.6 Button / ButtonEx —— 重写按钮样式](#36-button--buttonex--重写按钮样式)
  - [3.7 MultiCombo —— 多选下拉框](#37-multicombo--多选下拉框)
  - [3.8 BeginChild —— 修改签名的子窗口](#38-beginchild--修改签名的子窗口)
  - [3.9 BeginTable —— 修改签名的表格](#39-begintable--修改签名的表格)
- [4. 重写的 StyleColorsDark](#4-重写的-stylecolorsdark)
- [5. 字体系统](#5-字体系统)
- [6. 图片/纹理资源系统](#6-图片纹理资源系统)
- [7. 旋转绘制辅助函数](#7-旋转绘制辅助函数)
- [8. DrawUtils 绘制工具类](#8-drawutils-绘制工具类)
- [9. Notification 通知类](#9-notification-通知类)
- [10. MenuTooltip 工具提示](#10-menutooltip-工具提示)
- [11. 粒子背景系统](#11-粒子背景系统)
- [12. 搜索系统 (MenuSearch)](#12-搜索系统-menusearch)
- [13. Toast 通知系统](#13-toast-通知系统)
- [14. 菜单全局状态 (MenuSwitches)](#14-菜单全局状态-menuswitches)
- [15. 配置系统](#15-配置系统)
- [16. 在项目中添加新 UI 功能的完整流程](#16-在项目中添加新-ui-功能的完整流程)
- [附录 A: 原生 ImGui 控件在项目中的使用](#附录-a-原生-imgui-控件在项目中的使用)
- [附录 B: 颜色速查表](#附录-b-颜色速查表)

---

## 1. 魔改概述

本项目对 ImGui 1.89.6 进行了**深度侵入式修改**，直接改写了以下核心文件：

| 文件 | 修改内容 |
|------|----------|
| `imgui.h` | 新增 `Tab`、`SubTab`、`CustomButton`、`CheckPicker`、`MultiCombo` 声明；修改 `BeginChild`/`BeginTable` 签名 |
| `imgui.cpp` | 引入 `imgui_settings.h`；修改 `BeginChild`/`BeginChildEx`/`StyleColorsDark` 实现 |
| `imgui_widgets.cpp` | 引入 `imgui_settings.h`、`<map>`、`<d3d11.h>`；新增自定义控件；完全重写 `Checkbox`、`ButtonEx` |
| `imgui_draw.cpp` | 重写 `StyleColorsDark`（所有颜色 Alpha 设为 0） |
| `imgui_internal.h` | 修改 `BeginChildEx` 签名增加 `logo` 参数 |
| `imgui_settings.h` | **新增文件**：统一主题样式配置 |

**设计风格**：深色暗黑主题，暖橙色 `#FA8456` 为强调色，深灰 `#1B1D20` 为背景底色，所有组件统一暗灰+微圆角设计语言，动画使用 `ImLerp` 插值平滑过渡。

---

## 2. 主题与样式系统

### 2.1 配置文件位置

```
ImGui/imgui_settings.h
```

此文件定义在命名空间 `c::` 中，所有自定义控件的颜色、圆角、尺寸都引用此文件。**修改此文件即可全局更换主题**。

### 2.2 全局颜色常量

```cpp
namespace c
{
    inline ImVec4 accent_color       = ImColor(250, 132, 86);      // 主强调色（暖橙色 #FA8456）
    inline ImVec4 accent_low_color   = ImColor(250, 132, 86, 127); // 半透明强调色
    inline ImVec4 accent_low         = ImColor(174, 197, 255, 127); // 辅助半透明强调色（淡蓝）
    inline ImVec4 accent_text_color  = ImColor(245, 245, 255);     // 强调文字色（近白）
    inline ImVec4 accent_text_low_color = ImColor(245, 245, 255, 127); // 半透明强调文字色
    inline ImVec4 notify             = ImColor(43, 48, 54);        // 通知背景色（深灰）
}
```

**使用示例**：
```cpp
// 在绘制代码中使用强调色
ImGui::GetWindowDrawList()->AddRectFilled(pos, pos + size, ImGui::GetColorU32(c::accent_color), c::bg::rounding);
```

### 2.3 组件样式配置

每个组件都有统一的 `background`（背景色）、`outline_background`（边框色）、`rounding`（圆角）三个属性：

| 命名空间 | background | outline_background | rounding | 用途 |
|----------|-----------|-------------------|----------|------|
| `c::bg` | `ImColor(14,14,15,255)` | `ImColor(27,29,32,255)` | 12 | 主窗口背景，size=(750,640) |
| `c::child` | `ImColor(22,23,26,250)` | `ImColor(27,29,32,255)` | 6 | 子面板/Child 窗口 |
| `c::checkbox` | `ImColor(27,29,32,255)` | `ImColor(30,32,36,255)` | 30 | 自定义 Checkbox（胶囊形） |
| `c::slider` | `ImColor(27,29,32,255)` | `ImColor(30,32,36,255)` | 30 | 滑条 |
| `c::combo` | `ImColor(27,29,32,255)` | `ImColor(30,32,36,255)` | 3 | 下拉框 |
| `c::picker` | `ImColor(27,29,32,255)` | `ImColor(30,32,36,255)` | 2 | 颜色拾取器 |
| `c::button` | `ImColor(27,29,32,255)` | `ImColor(30,32,36,255)` | 4 | 按钮 |
| `c::input` | `ImColor(27,29,32,255)` | `ImColor(30,32,36,255)` | 4 | 输入框 |
| `c::keybind` | `ImColor(27,29,32,255)` | `ImColor(30,32,36,255)` | 3 | 快捷键绑定控件 |

**额外属性**：
```cpp
c::bg::size           = ImVec2(750, 640);  // 主窗口固定尺寸
c::checkbox::circle_inactive = ImColor(43, 48, 54, 255);  // 开关圆形滑块未激活色
c::slider::circle_inactive   = ImColor(43, 48, 54, 255);  // 滑条圆形滑块未激活色
```

### 2.4 文字样式

```cpp
namespace c::text
{
    inline ImVec4 text_hov  = ImColor(245, 245, 255);   // 悬停态文字色（亮白）
    inline ImVec4 text      = ImColor(90, 93, 100);     // 默认灰色文字
    inline ImVec4 text2     = ImColor(90, 93, 100, 0);  // 透明文字（隐藏用）
    inline ImVec4 hide_text = ImColor(43, 48, 54, 255); // 占位/隐藏文字色
}
```

### 2.5 修改主题色

要全局修改主题，修改 `imgui_settings.h` 中的 `accent_color` 即可：

```cpp
// 示例：改为蓝色主题
inline ImVec4 accent_color = ImColor(86, 156, 250);  // #569CFA
```

所有引用 `c::accent_color` 的控件（Tab 图标、Checkbox 滑块、Button 激活态等）都会自动跟随变化。

---

## 3. 自定义控件详解

### 3.1 Tab —— 自定义标签页按钮

**声明**（`imgui_widgets.cpp:1050`，未在 `imgui.h` 中显式声明但可用）：

```cpp
bool ImGui::Tab(bool selected, const char* icon, const char* label, const char* hint, const ImVec2& size_arg);
```

**参数说明**：

| 参数 | 类型 | 说明 |
|------|------|------|
| `selected` | `bool` | 当前是否被选中 |
| `icon` | `const char*` | 图标字符（使用 `font::pTabIcon` 字体渲染，如 `"E"`, `"D"`, `"A"` 等） |
| `label` | `const char*` | 标签文字（如 `"瞄准"`, `"视觉"`） |
| `hint` | `const char*` | 提示文字（如 `"Aim, Recoil, Trigger"`） |
| `size_arg` | `const ImVec2&` | 按钮尺寸 |

**特性**：
- 左侧图标（使用 `font::pTabIcon` 字体）+ 右侧文字标签 + 底部提示文字
- 使用 `tab_state` 结构体 + `ImLerp` 实现平滑颜色过渡动画
- 选中/悬停时：图标变 `c::accent_color`，文字变 `c::accent_text_color`
- 未选中时：图标灰色，文字分悬停（`c::text::text_hov`）/非悬停（`c::text::text`）两种灰度
- 动画速度：`g.IO.DeltaTime * 12.f`
- 内部强制 `ItemSpacing = ImVec2(0, 30)` 撑开垂直间距

**使用示例**（来自 `Menu.cpp:1472-1480`）：

```cpp
const char* nametab_array1[6] = { "E", "D", "A", "G", "C", "B" };
const char* nametab_array[6]  = { "瞄准", "视觉", "颜色", "内存", "设置", "配置" };
const char* nametab_hint_array[6] = { "Aim, Recoil, Trigger", "Overlay, Chams, World", "Game Colors", "World Memory", "Element Setup", "Save Settings" };

static int tabs = 0;

ImGui::BeginGroup();
{
    for (int i = 0; i < 6; i++)
        if (ImGui::Tab(i == tabs, nametab_array1[i], nametab_array[i], nametab_hint_array[i], ImVec2(35 + tab_size, 35)))
            tabs = i;
}
ImGui::EndGroup();
```

**注意事项**：
- `icon` 参数对应的字符映射由 `font::pTabIcon` 字体文件决定
- `tab_size` 变量控制侧边栏展开宽度，实现展开/收起动画

---

### 3.2 SubTab —— 子标签页图片按钮

**声明**（`imgui_widgets.cpp:1104`）：

```cpp
bool ImGui::SubTab(bool selected, ImTextureID image, const ImVec2& size_arg);
```

**参数说明**：

| 参数 | 类型 | 说明 |
|------|------|------|
| `selected` | `bool` | 当前是否被选中 |
| `image` | `ImTextureID` | 纹理 ID（`ID3D11ShaderResourceView*`） |
| `size_arg` | `const ImVec2&` | 按钮尺寸 |

**特性**：
- 纯图片按钮，使用 `AddImage` 渲染
- 同样使用 `tab_state` 动画插值
- 选中/悬停时图片着色为 `c::accent_color`
- `image` 为 `nullptr` 时不绘制

**使用示例**：

```cpp
if (ImGui::SubTab(isActive, myTextureSRV, ImVec2(40, 40)))
    currentSubTab = i;
```

---

### 3.3 CustomButton —— 自定义图片按钮

**声明**（`imgui_widgets.cpp:1529`）：

```cpp
bool ImGui::CustomButton(ImGuiID id, ImTextureID texture_id, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, ImU32 color);
```

**参数说明**：

| 参数 | 类型 | 说明 |
|------|------|------|
| `id` | `ImGuiID` | 控件唯一 ID（注意：直接接受 `ImGuiID` 而非字符串标签） |
| `texture_id` | `ImTextureID` | 纹理 ID |
| `size` | `const ImVec2&` | 按钮尺寸 |
| `uv0` | `const ImVec2&` | UV 左上角（默认 `ImVec2(0,0)`） |
| `uv1` | `const ImVec2&` | UV 右下角（默认 `ImVec2(1,1)`） |
| `color` | `ImU32` | 着色颜色 |

**特性**：
- 直接接受 `ImGuiID` 而非字符串标签，更灵活
- 简洁实现：仅渲染纹理图片 + `ButtonBehavior` 交互
- 常用于侧边栏收起按钮等场景

**使用示例**（来自 `Menu.cpp:1492`）：

```cpp
// 配合旋转动画使用
ImRotateStart();
if (ImGui::CustomButton(1, image::roll, ImVec2(35, 35), ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(c::accent_color)))
    tab_opening = !tab_opening;
ImRotateEnd(1.57f * arrow_roll);
```

---

### 3.4 Checkbox —— 重写开关样式

**声明**（`imgui.h:506`，实现被完全重写 `imgui_widgets.cpp:1593`）：

```cpp
bool ImGui::Checkbox(const char* label, bool* v);
```

**签名与原生相同，但渲染完全不同！**

**重写内容**：
- 使用自定义**开关样式（Switch Toggle）**，而非默认方形勾选框
- 绘制圆角矩形背景（`c::checkbox::background`，rounding=30 → 胶囊形）
- 内部绘制圆形滑块（`AddCircleFilled`），开启时向右滑动
- 开启时圆形滑块带阴影效果（`AddShadowCircle`）
- 使用 `check_state` 结构体 + `ImLerp` 实现平滑动画：
  - `circle`：圆形滑块颜色（激活 → `c::accent_color`，未激活 → `c::checkbox::circle_inactive`）
  - `text`：文字颜色（激活 → `c::accent_text_color`，悬停 → `c::text::text_hov`，默认 → `c::text::text`）
  - `move`：滑块位移（激活 → 9.f，未激活 → -12.f）

**使用示例**（与原生相同）：

```cpp
bool myFlag = false;
ImGui::Checkbox("启用自瞄", &myFlag);
MenuTooltip("自瞄总开关，打开后自瞄功能开始工作");
```

---

### 3.5 CheckPicker —— 复选框+颜色拾取器组合

**声明**（`imgui.h:529`，实现 `imgui_widgets.cpp:1645`）：

```cpp
void ImGui::CheckPicker(const char* label, const char* label_picker, bool* v, float col[4]);
```

**参数说明**：

| 参数 | 类型 | 说明 |
|------|------|------|
| `label` | `const char*` | 复选框标签 |
| `label_picker` | `const char*` | 颜色拾取器标签（需唯一） |
| `v` | `bool*` | 复选框绑定变量 |
| `col` | `float[4]` | 颜色值（RGBA，0.0~1.0） |

**特性**：
- 右侧显示 `ColorEdit4` 拾取器，左侧显示自定义 `Checkbox`
- 通过光标位置偏移实现左右布局
- 适合"开关+颜色"组合配置场景

**使用示例**：

```cpp
bool showBox = true;
float boxColor[4] = { 0.3f, 0.7f, 1.0f, 1.0f };
ImGui::CheckPicker("方框 ESP", "方框颜色", &showBox, boxColor);
```

---

### 3.6 Button / ButtonEx —— 重写按钮样式

**声明**（`imgui_widgets.cpp:1150`）：

```cpp
bool ImGui::ButtonEx(const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags);
bool ImGui::Button(const char* label, const ImVec2& size_arg = ImVec2(0, 0));
```

**重写内容**：
- 使用 `button_state` 结构体 + `ImLerp` 实现平滑动画
- 绘制自定义圆角矩形背景（`c::button::background`，rounding=4）
- 绘制自定义边框（`c::button::outline_background`）
- 激活态背景色变亮，文字色变为 `c::accent_color`
- 文字居中对齐

**使用示例**（与原生相同）：

```cpp
if (ImGui::Button("提交参数", ImVec2(220, 0)))
{
    // 点击回调
}
```

---

### 3.7 MultiCombo —— 多选下拉框

**声明**（`imgui.h:526`，实现 `imgui_widgets.cpp:2461`）：

```cpp
void ImGui::MultiCombo(const char* label, bool variable[], const char* labels[], int count);
```

**参数说明**：

| 参数 | 类型 | 说明 |
|------|------|------|
| `label` | `const char*` | 控件标签 |
| `variable` | `bool[]` | 各选项的选中状态数组 |
| `labels` | `const char*[]` | 各选项的显示名称数组 |
| `count` | `int` | 选项数量 |

**特性**：
- 支持同时选中多个选项
- 预览文本动态拼接已选项目名称（用逗号分隔），无选中时显示 "None"
- 使用 `ImGuiSelectableFlags_DontClosePopups` 保持弹出框打开

**使用示例**：

```cpp
bool filters[3] = { true, false, true };
const char* filterNames[3] = { "僵尸", "吸血鬼", "队友" };
ImGui::MultiCombo("过滤目标", filters, filterNames, 3);
```

---

### 3.8 BeginChild —— 修改签名的子窗口

**声明**（`imgui.h:342`）：

```cpp
bool ImGui::BeginChild(const char* str_logo, const char* str_id, const ImVec2& size = ImVec2(0, 0), bool border = false, ImGuiWindowFlags flags = 0);
```

**与原生的差异**：

| 对比项 | 原生 ImGui 1.89.6 | 本项目魔改版 |
|--------|-------------------|-------------|
| 第一个参数 | `const char* str_id` | `const char* str_logo` |
| 第二个参数 | `const ImVec2& size` | `const char* str_id` |
| 内部样式 | 默认 | 自定义 `WindowPadding = ImVec2(20, 13)`，`ItemSpacing = ImVec2(10, 10)` |
| 内部标志 | 默认 | 自动添加 `NoBackground \| AlwaysUseWindowPadding` |

**使用示例**（来自 `Menu.cpp:1542`）：

```cpp
// 注意：第一个参数是 logo/icon 标识，第二个才是真正的 ID
ImGui::BeginChild("D", "Main", ImVec2((c::bg::size.x - 60 - s->ItemSpacing.x * 4) + 18, 405));
{
    DrawAimbotTab();
}
ImGui::EndChild();
```

**注意**：`EndChild()` 会自动 `PopStyleVar(2)` 还原内边距和间距。

---

### 3.9 BeginTable —— 修改签名的表格

**声明**（`imgui.h:745`）：

```cpp
bool ImGui::BeginTable(const char* str_logo, const char* str_id, int column, ImGuiTableFlags flags = 0, const ImVec2& outer_size = ImVec2(0.0f, 0.0f), float inner_width = 0.0f);
```

**与原生的差异**：新增 `str_logo` 参数作为第一个参数，其余签名不变。

---

## 4. 重写的 StyleColorsDark

项目对 `StyleColorsDark` 进行了修改：**所有颜色的 Alpha 通道设为 0**（完全透明），使得 ImGui 原生样式完全不渲染，所有可见样式均由自定义绘制代码提供。

```cpp
// imgui_draw.cpp 中的修改示例
colors[ImGuiCol_Text]           = ImVec4(1.00f, 1.00f, 1.00f, 0.f);  // 原生 alpha=1.00
colors[ImGuiCol_WindowBg]       = ImVec4(0.06f, 0.06f, 0.06f, 0.f);  // 原生 alpha=0.87
colors[ImGuiCol_FrameBg]        = ImVec4(0.16f, 0.29f, 0.48f, 0.f);  // 原生 alpha=1.00
// ... 所有颜色 alpha = 0
```

**这意味着**：
- 所有背景、边框、滑条等都由自定义绘制代码处理
- 修改外观应改 `imgui_settings.h`，而非 `ImGuiStyle`
- 原生 `PushStyleColor` 仍可用，但需要手动设置非零 alpha

---

## 5. 字体系统

### 5.1 字体定义位置

**定义**：`src/Hook/DX11Hook.cpp:38-46`
**声明**：`src/UI/Menu/Menu.cpp:22-30`

```cpp
namespace font
{
    ImFont* pPoppinsMedium    = nullptr;  // Poppins Medium 17px（英文主字体）
    ImFont* pPoppinsMediumLow = nullptr;  // Poppins Medium 15px（英文小号字体）
    ImFont* pTabIcon          = nullptr;  // Tab 图标字体 24px（图标映射字体）
    ImFont* pChicons          = nullptr;  // 图标字体 20px
    ImFont* pTahomaBold       = nullptr;  // Tahoma Bold 17px（支持中文）
    ImFont* pTahomaBold2      = nullptr;  // Tahoma Bold 28px（大号标题，支持中文）
}
```

### 5.2 字体加载流程

在 `DX11Hook.cpp` 的 ImGui 初始化阶段加载：

1. **中文字体**（优先级从高到低）：
   - `C:\Windows\Fonts\msyh.ttc`（微软雅黑）
   - `C:\Windows\Fonts\msyh.ttf`
   - `C:\Windows\Fonts\simhei.ttf`（黑体）
   - `C:\Windows\Fonts\simsun.ttc`（宋体）
   - `C:\Windows\Fonts\Deng.ttf`（等线）
   - 以上均失败则回退到默认字体
   - 大小：18px，使用 `GetGlyphRangesChineseFull()` 字符范围

2. **自定义字体**（从内存字节数组加载）：
   - `poppins_medium` → `pPoppinsMedium`(17px) / `pPoppinsMediumLow`(15px)
   - `tabs_icons` → `pTabIcon`(24px)，使用 `GetGlyphRangesCyrillic()`
   - `tahoma_bold` → `pTahomaBold`(17px) / `pTahomaBold2`(28px)，使用中文字符范围
   - `chicon` → `pChicons`(20px)，使用 `GetGlyphRangesCyrillic()`

3. **保护措施**：加载失败时回退到 `io.FontDefault`

### 5.3 字体使用示例

```cpp
// 临时切换字体
if (font::pTahomaBold2) ImGui::PushFont(font::pTahomaBold2);
ImGui::Text("大标题");
if (font::pTahomaBold2) ImGui::PopFont();

// Tab 控件内部使用图标字体
ImGui::PushFont(font::pTabIcon);
ImGui::GetWindowDrawList()->AddText(pos, color, iconChar);
ImGui::PopFont();
```

---

## 6. 图片/纹理资源系统

### 6.1 纹理定义

**定义**：`src/Hook/DX11Hook.cpp:48-56`
**声明**：`src/UI/Menu/Menu.cpp:32-41`

```cpp
namespace image
{
    ID3D11ShaderResourceView* bg           = nullptr;  // 背景贴图
    ID3D11ShaderResourceView* logo         = nullptr;  // Logo 图标
    ID3D11ShaderResourceView* logo_general = nullptr;  // 通用 Logo
    ID3D11ShaderResourceView* arrow        = nullptr;  // 箭头图标
    ID3D11ShaderResourceView* bell_notify  = nullptr;  // 通知铃铛图标
    ID3D11ShaderResourceView* roll         = nullptr;  // 旋转/展开图标
}
```

### 6.2 纹理加载

- 使用 `#ifdef USE_EMBEDDED_IMAGES` 条件编译
- 从内嵌字节数组加载（定义在 `image.h` 中）
- 加载函数：`LoadTextureFromMemory()`
- 可用 `Scripts/image_to_cpp.py` 将 PNG/JPG 转换为 C++ 字节数组

### 6.3 纹理使用示例

```cpp
// 绘制背景贴图
if (image::bg)
{
    ImGui::GetBackgroundDrawList()->AddImage(
        image::bg, ImVec2(0, 0), ImVec2(1920, 1080),
        ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255));
}

// 绘制 Logo
if (image::logo)
{
    ImGui::GetWindowDrawList()->AddImage(
        image::logo, pos + ImVec2(14, 14), pos + ImVec2(46, 46),
        ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255));
}
```

---

## 7. 旋转绘制辅助函数

定义在 `Menu.cpp:43-70`，用于实现侧边栏展开按钮的旋转动画。

### 7.1 API

```cpp
void ImRotateStart();                                    // 记录当前顶点缓冲起点
ImVec2 ImRotationCenter();                                // 计算本轮新增图元的包围盒中心
void ImRotateEnd(float rad, ImVec2 center = ImRotationCenter()); // 对新增顶点批量旋转
```

### 7.2 使用示例

```cpp
ImRotateStart();
if (ImGui::CustomButton(1, image::roll, ImVec2(35, 35), ImVec2(0, 0), ImVec2(1, 1), color))
    tab_opening = !tab_opening;
ImRotateEnd(1.57f * arrow_roll);  // 1.57 ≈ π/2，即 90° 旋转
```

---

## 8. DrawUtils 绘制工具类

**头文件**：`src/UI/Draw/Draw.h`
**实现**：`src/UI/Draw/Draw.cpp`

对 ImGui DrawList 的轻量封装，统一颜色类型转换。

### 8.1 API

```cpp
class DrawUtils
{
public:
    static void DrawLine(const Vector2& start, const Vector2& end, ImU32 color, float thickness = 1.0f);
    static void DrawRect(const Vector2& topLeft, const Vector2& bottomRight, ImU32 color, float thickness = 1.0f, bool filled = false);
    static void DrawCircle(const Vector2& center, float radius, ImU32 color, int numSegments = 32, float thickness = 1.0f);
    static void DrawText(const Vector2& pos, const char* text, ImU32 color, ImFont* font = nullptr);
    static void DrawTextWithBackground(const Vector2& pos, const char* text, ImU32 textColor, ImU32 bgColor, ImFont* font = nullptr);
    static ImU32 ColorToImU32(const ImColor_RGBA& color);   // RGBA 0.0~1.0 → ImU32
    static ImColor_RGBA ImU32ToColor(ImU32 imcolor);         // ImU32 → RGBA 0.0~1.0
};
```

### 8.2 使用示例

```cpp
// 绘制方框 ESP
DrawUtils::DrawRect(Vector2(x, y), Vector2(x + w, y + h), IM_COL32(255, 0, 0, 255), 2.0f);

// 绘制带背景文字
DrawUtils::DrawTextWithBackground(
    Vector2(screenX, screenY), "Enemy",
    IM_COL32(255, 255, 255, 255),    // 白色文字
    IM_COL32(0, 0, 0, 128));          // 半透明黑色背景

// 颜色转换
ImColor_RGBA rgba(1.0f, 0.0f, 0.0f, 1.0f);  // 红色
ImU32 color = DrawUtils::ColorToImU32(rgba);  // 转 ImU32
```

---

## 9. Notification 通知类

**头文件**：`src/UI/Draw/Draw.h`
**实现**：`src/UI/Draw/Draw.cpp`

右下角轻量提示，带淡出动画。

### 9.1 API

```cpp
class Notification
{
public:
    Notification();
    ~Notification();
    void Show(const char* text, ImU32 color, float duration = 3.0f);
    void Hide();
    void Update();     // 每帧调用
    void Draw();       // 每帧调用
    bool IsVisible() const;
};
```

### 9.2 使用示例

```cpp
Notification notify;

// 显示通知
notify.Show("配置保存成功", IM_COL32(0, 255, 0, 255), 3.0f);

// 每帧更新和绘制
notify.Update();
notify.Draw();
```

**淡出机制**：末尾 0.5 秒做 alpha 淡出。

---

## 10. MenuTooltip 工具提示

**定义**：`Menu.cpp:115-151`（`Menu` 命名空间内的 static 函数）

### 10.1 API

```cpp
static void MenuTooltip(const char* text);
```

### 10.2 使用模式

紧跟在 ImGui 控件之后调用：

```cpp
ImGui::Checkbox("开启自瞄", &MenuSwitches::g_Toggles.OpenAimbot);
MenuTooltip("自瞄总开关，打开后自瞄功能开始工作");
```

### 10.3 特性

- 受 `MenuSwitches::g_Toggles.ShowTooltips` 全局开关控制
- 使用前台绘制列表（`GetForegroundDrawList`），保证 tooltip 在所有层之上
- 自动防超出屏幕
- 深灰半透明圆角背景 + 边框
- 鼠标右下方偏移 16px 定位

---

## 11. 粒子背景系统

**定义**：`Menu.cpp:72-107`

### 11.1 控制方式

```cpp
// 性能模式下自动关闭粒子
if (MenuSwitches::g_Toggles.PerformanceMode)
    return;  // 跳过粒子绘制
```

### 11.2 特性

- 50 个粒子，从屏幕顶部随机出生
- 使用 `ImLerp` 线性插值下落
- 颜色：`ImColor(71, 226, 67, 255/2)`（半透明绿色）
- 到达底部后重新从顶部出生
- 半径 0~4 随机

---

## 12. 搜索系统 (MenuSearch)

**头文件**：`src/UI/Menu/MenuSearch.h`

### 12.1 核心功能

- 支持中文/英文关键词搜索
- 模糊匹配，多权重打分
- 搜索结果高亮
- 自动跳转到对应 Tab 页

### 12.2 API

```cpp
// 执行搜索
int MenuSearch::Search(const char* query, std::vector<SearchResult>& results, int maxResults = 10);

// 获取 Tab 名称
const char* MenuSearch::GetTabName(int tabIndex);

// 可搜索项数据库
static const SearchableItem g_searchDatabase[];
static constexpr size_t g_searchDatabaseSize;
```

### 12.3 搜索结果结构

```cpp
struct SearchResult
{
    const SearchableItem* item;
    int matchScore;  // 匹配分数，越高越相关
};

struct SearchableItem
{
    const char* displayName;    // 显示名称（中文）
    const char* englishName;    // 英文名称
    const char* keywords;       // 关键词（空格分隔）
    const char* description;    // 功能描述
    int         tabIndex;       // 所属 Tab 页索引
    const char* category;       // 分类
};
```

### 12.4 匹配权重

| 字段 | 权重 |
|------|------|
| displayName（中文名） | 100 |
| englishName（英文名） | 80 |
| keywords | 50 |
| description | 30 |
| category | 10 |

### 12.5 使用示例

```cpp
static char Search[256] = { "" };
static std::vector<MenuSearch::SearchResult> searchResults;

if (ImGui::InputTextWithHint("搜索", "搜索功能...", Search, 256))
{
    if (Search[0] != '\0')
        MenuSearch::Search(Search, searchResults, 8);
}

// 显示搜索结果
for (const auto& result : searchResults)
{
    if (ImGui::Selectable(result.item->displayName))
        tabs = result.item->tabIndex;
}
```

---

## 13. Toast 通知系统

**定义**：`Menu.cpp:168-215`（`Menu` 命名空间内的匿名命名空间）

### 13.1 API

```cpp
// 显示 Toast（内部函数）
void ShowToast(const char* text, const ImVec4& color = ImVec4(0.95f, 0.95f, 0.95f, 1.0f), double durationSeconds = 2.0);

// 绘制 Toast（在 Render() 末尾调用）
void DrawToast();
```

### 13.2 使用示例

```cpp
Menu::ShowToast("配置保存成功", ImVec4(0.45f, 0.95f, 0.55f, 1.0f));  // 绿色
Menu::ShowToast("操作失败", ImVec4(1.00f, 0.45f, 0.45f, 1.0f));      // 红色
```

### 13.3 特性

- 右上角定位
- 自动过期（默认 2 秒）
- 半透明背景（0.90）
- 无装饰无边框

---

## 14. 菜单全局状态 (MenuSwitches)

**头文件**：`src/UI/Menu/MenuSwitches.h`
**实现**：`src/UI/Menu/MenuSwitches.cpp`

### 14.1 核心结构

```cpp
namespace MenuSwitches
{
    struct ToggleState
    {
        // === 自瞄页 (Aim) ===
        bool  OpenAimbot    = false;
        bool  BoneOn        = false;
        bool  MemoryAimbot  = false;
        bool  Missed_shot   = false;
        float Aim_Range     = 120.0f;
        float threshold     = 8.0f;
        bool  TriggerBot    = false;
        bool  AimPrediction = false;
        bool  AimFovLimit   = true;
        float AimFov        = 35.0f;
        float AimSmooth     = 5.0f;
        int   AimBoneIndex  = 0;

        // === 视觉页 (ESP) ===
        bool  EspBox / EspName / EspHealth / EspDistance / EspSkeleton / EspSnapline / EspLoot / EspRadar = false;
        float EspMaxDistance = 250.0f;
        float EspThickness   = 1.5f;
        float EspTextScale   = 1.0f;
        float EspColor[4]    = { 1.0f, 0.55f, 0.30f, 1.0f };
        bool  EspFillBox     = false;
        bool  EspCornerBox   = false;
        bool  FilterDead     = true;

        // === 颜色页 (Color) ===
        float AccentHue      = 0.06f;
        float GlobalAlpha    = 0.95f;
        bool  DynamicGradient = false;
        float ColorBox2D[4], ColorBox3D[4], ColorName[4], ColorSnapline[4], ...
        float ColorDistance[4], ColorHealth[4], ColorBoneVisible[4], ColorBoneOccluded[4], ColorRadar[4], ColorLoot[4];

        // === 内存页 (Memory) ===
        bool  PreviewNoRecoil / PreviewRapidFire / PreviewNoSpread / PreviewSpeedHack / PreviewInfiniteAmmo = false;
        float PreviewSpeedScale = 1.2f;
        int   PreviewAmmoValue  = 999;
        bool  PreviewWorldObjects / PreviewEntityInfo / DrawObjectPointers = false;

        // === 设置页 (Settings) ===
        bool  ShowTooltips / ShowFps / ShowGameFps / ShowOverlayFps = true;
        float MenuAlpha       = 0.95f;
        int   ThemeIndex      = 0;
        float MaxFps          = 144.0f;
        bool  Vsync / PerformanceMode / StreamProof / PanicKeyEnabled = ...;
        float UiScale         = 1.0f;

        // === 配置页 (Config) ===
        bool  AutoSavePreview = false;
        bool  AutoLoadPreview = false;
    };

    extern ToggleState g_Toggles;   // 全局状态实例
    void ResetDefaults();            // 重置为默认值
}
```

### 14.2 使用方式

所有 ImGui 控件直接绑定到 `g_Toggles` 的字段：

```cpp
ImGui::Checkbox("开启自瞄", &MenuSwitches::g_Toggles.OpenAimbot);
ImGui::SliderFloat("自瞄范围", &MenuSwitches::g_Toggles.Aim_Range, 1.f, 500.f, "%.1f");
ImGui::ColorEdit4("2D方框颜色", MenuSwitches::g_Toggles.ColorBox2D, picker_flags);
```

---

## 15. 配置系统

配置文件使用 JSON 序列化，支持手动和自动保存/加载。

### 15.1 文件位置

| 文件 | 路径 | 用途 |
|------|------|------|
| 主配置 | `config/settings.json` | 手动保存/加载 |
| 自动配置 | `config/settings_auto.json` | 自动读取/保存策略 |

### 15.2 API

```cpp
namespace Config
{
    bool SaveConfig(const char* filename);
    bool LoadConfig(const char* filename);
    bool SaveAutoSettings(const char* filename, bool autoLoad, bool autoSave);
}
```

---

## 16. 在项目中添加新 UI 功能的完整流程

### 16.1 添加新的布尔开关

1. **在 `MenuSwitches.h` 的 `ToggleState` 中添加字段**：
   ```cpp
   bool InfiniteHealth = false;  // 无限血量开关
   ```

2. **在 `Menu.cpp` 对应 Tab 中添加控件**：
   ```cpp
   ImGui::Checkbox("无限血量", &MenuSwitches::g_Toggles.InfiniteHealth);
   MenuTooltip("血量不减少");
   ```

3. **在 `MenuSearch.h` 的 `g_searchDatabase` 中添加搜索项**：
   ```cpp
   { "无限血量", "Infinite Health", "血量 无限 health infinite", "血量不减少", 3, "内存" },
   ```

4. **（可选）在 `MenuFeatureStubs` 中添加预留接口**。

### 16.2 添加新的浮点参数

1. **在 `ToggleState` 中添加**：
   ```cpp
   float BulletSpeedScale = 1.0f;
   ```

2. **在 Menu.cpp 中添加滑条**：
   ```cpp
   ImGui::SliderFloat("子弹速度倍率", &MenuSwitches::g_Toggles.BulletSpeedScale, 0.5f, 5.0f, "%.2f");
   MenuTooltip("子弹飞行速度倍率，1.0=正常");
   ```

### 16.3 添加新的颜色项

1. **在 `ToggleState` 中添加**：
   ```cpp
   float ColorHeadshot[4] = {1.0f, 0.0f, 0.0f, 1.0f};
   ```

2. **在颜色页 (`DrawOverviewTab`) 中添加**：
   ```cpp
   const ImGuiColorEditFlags picker_flags = ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaPreview;
   ImGui::ColorEdit4("爆头特效颜色", MenuSwitches::g_Toggles.ColorHeadshot, picker_flags);
   MenuTooltip("爆头时的特效颜色");
   ```

### 16.4 添加新的 Tab 页面

1. **在 `Menu.h` 中声明**：
   ```cpp
   void DrawNewTab();
   ```

2. **在 `Menu.cpp` 中实现**：
   ```cpp
   void DrawNewTab()
   {
       ImGui::TextUnformatted("新功能 - New Feature");
       ImGui::Separator();
       ImGui::Spacing();
       // 添加控件...
   }
   ```

3. **在 `Render()` 中添加**：
   ```cpp
   // Tab 数组增加一项
   const char* nametab_array1[7] = { "E", "D", "A", "G", "C", "B", "F" };
   const char* nametab_array[7]  = { "瞄准", "视觉", "颜色", "内存", "设置", "配置", "新功能" };
   const char* nametab_hint_array[7] = { ..., "New Feature" };

   // if (tabs == 6) 分支中调用
   if (tabs == 6)
   {
       ImGui::SetCursorPos(ImVec2(60 + tab_size, 60) + (s->ItemSpacing * 2));
       ImGui::BeginGroup();
       {
           ImGui::BeginChild("D", "Main", ImVec2(...));
           { DrawNewTab(); }
           ImGui::EndChild();
       }
       ImGui::EndGroup();
   }
   ```

4. **更新搜索系统**：修改 `MenuSearch.h` 中的 `tabIndex` 和 `GetTabName`。

---

## 附录 A: 原生 ImGui 控件在项目中的使用

本项目仍然使用以下原生 ImGui 控件（但部分内部实现已被修改）：

| 控件 | 使用方式 | 是否被修改 |
|------|---------|-----------|
| `Checkbox` | `ImGui::Checkbox(label, &v)` | **是**（改为开关样式） |
| `Button` | `ImGui::Button(label, size)` | **是**（自定义样式） |
| `SliderFloat` | `ImGui::SliderFloat(label, &v, min, max, fmt)` | 否（样式由 StyleColorsDark 透明化） |
| `SliderInt` | `ImGui::SliderInt(label, &v, min, max, fmt)` | 否 |
| `ColorEdit4` | `ImGui::ColorEdit4(label, col[4], flags)` | 否 |
| `InputTextWithHint` | `ImGui::InputTextWithHint(label, hint, buf, size)` | 否 |
| `Combo` | `ImGui::Combo(label, &current, items, count)` | 否 |
| `BeginChild`/`EndChild` | `ImGui::BeginChild(str_logo, str_id, size)` | **是**（签名修改+自定义内边距） |
| `BeginTable`/`EndTable` | `ImGui::BeginTable(str_logo, str_id, cols)` | **是**（签名修改） |
| `Text`/`TextDisabled`/`TextColored` | 各种文字显示 | 否 |
| `Separator`/`SeparatorText`/`Spacing` | 布局辅助 | 否 |
| `Selectable` | `ImGui::Selectable(label, &selected, flags)` | 否 |
| `SameLine`/`Indent`/`Unindent` | 布局辅助 | 否 |
| `SetCursorPos`/`GetCursorPos` | 光标控制 | 否 |
| `PushFont`/`PopFont` | 字体切换 | 否 |
| `PushStyleColor`/`PopStyleColor` | 颜色栈 | 否 |
| `PushStyleVar`/`PopStyleVar` | 样式栈 | 否 |
| `PushItemWidth`/`PopItemWidth` | 宽度栈 | 否 |

---

## 附录 B: 颜色速查表

### 强调色系

| 名称 | RGB | Hex | 用途 |
|------|-----|-----|------|
| `c::accent_color` | (250, 132, 86) | `#FA8456` | 主强调色（选中/激活态） |
| `c::accent_low_color` | (250, 132, 86, 127) | `#FA845680` | 半透明强调色 |
| `c::accent_low` | (174, 197, 255, 127) | `#AEC5FF80` | 辅助半透明强调色 |
| `c::accent_text_color` | (245, 245, 255) | `#F5F5FF` | 强调文字色 |
| `c::accent_text_low_color` | (245, 245, 255, 127) | `#F5F5FF80` | 半透明强调文字色 |

### 背景色系

| 名称 | RGB | Hex | 用途 |
|------|-----|-----|------|
| `c::bg::background` | (14, 14, 15) | `#0E0E0F` | 主窗口背景 |
| `c::bg::outline_background` | (27, 29, 32) | `#1B1D20` | 主窗口边框/背景底色 |
| `c::child::background` | (22, 23, 26) | `#16171A` | 子面板背景 |
| `c::child::outline_background` | (27, 29, 32) | `#1B1D20` | 子面板边框 |

### 组件色系

| 名称 | RGB | Hex | 用途 |
|------|-----|-----|------|
| `c::checkbox::background` | (27, 29, 32) | `#1B1D20` | Checkbox/Slider/Combo 等背景 |
| `c::checkbox::outline_background` | (30, 32, 36) | `#1E2024` | Checkbox/Slider/Combo 等边框 |
| `c::checkbox::circle_inactive` | (43, 48, 54) | `#2B3036` | 开关圆形滑块未激活色 |

### 文字色系

| 名称 | RGB | Hex | 用途 |
|------|-----|-----|------|
| `c::text::text_hov` | (245, 245, 255) | `#F5F5FF` | 悬停态文字（亮白） |
| `c::text::text` | (90, 93, 100) | `#5A5D64` | 默认灰色文字 |
| `c::text::hide_text` | (43, 48, 54) | `#2B3036` | 占位/隐藏文字 |
| `c::notify` | (43, 48, 54) | `#2B3036` | 通知背景色 |
