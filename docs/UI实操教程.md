好的！这是完整的UI实操教程文档内容，你可以直接复制粘贴：

```markdown
# UI菜单实操教程 - 手把手教你添加新功能

## 📋 目录
1. [快速开始](#快速开始)
2. [添加简单复选框](#添加简单复选框)
3. [添加滑块控件](#添加滑块控件)
4. [添加颜色选择器](#添加颜色选择器)
5. [添加下拉框](#添加下拉框)
6. [添加按钮](#添加按钮)
7. [创建新的标签页](#创建新的标签页)
8. [配置系统集成](#配置系统集成)
9. [完整案例](#完整案例)

---

## 快速开始

### UI系统文件结构

```
src/
├── UI/
│   ├── Menu/
│   │   ├── Menu.cpp              # 主菜单渲染逻辑
│   │   ├── Menu.h                # 菜单函数声明
│   │   ├── MenuSwitches.h        # 所有开关和变量定义
│   │   └── MenuSwitches.cpp      # 变量初始化
│   └── Draw/
│       └── Draw.cpp              # 绘制辅助函数
├── Config/
│   ├── Config.cpp                # 配置保存/加载
│   ├── Config.h                  # 配置函数声明
│   └── ConfigSchema.h            # 配置结构定义
└── Core/
    └── globals.h                 # 全局变量（如需要）
```

### 添加新功能的基本流程

```
1. 在 MenuSwitches.h 中声明变量
   ↓
2. 在 MenuSwitches.cpp 中初始化变量
   ↓
3. 在 Menu.cpp 中添加 UI 控件
   ↓
4. 在 ConfigSchema.h 中添加配置项
   ↓
5. 编译测试
```

---

## 添加简单复选框

### 案例：添加"无限跳跃"功能

#### 步骤1：在 MenuSwitches.h 中声明变量

打开 `src/UI/Menu/MenuSwitches.h`，找到 `ToggleState` 结构体，添加你的变量：

```cpp
// MenuSwitches.h
struct ToggleState
{
    // === 现有变量 ===
    bool OpenAimbot;
    bool BoneOn;
    // ... 其他变量

    // === 添加你的新变量 ===
    bool InfiniteJump;        // 无限跳跃开关
};
```

**命名规范：**
- 使用驼峰命名法（CamelCase）
- bool 类型用动词或形容词开头
- 清晰表达功能含义

#### 步骤2：在 MenuSwitches.cpp 中初始化

打开 `src/UI/Menu/MenuSwitches.cpp`，找到 `ResetDefaults()` 函数：

```cpp
// MenuSwitches.cpp
void ResetDefaults()
{
    // === 现有初始化 ===
    g_Toggles.OpenAimbot = false;
    g_Toggles.BoneOn = false;
    // ... 其他初始化

    // === 添加你的初始化 ===
    g_Toggles.InfiniteJump = false;  // 默认关闭
}
```

#### 步骤3：在 Menu.cpp 中添加 UI 控件

打开 `src/UI/Menu/Menu.cpp`，找到你想添加功能的标签页函数。

**例如在"内存"标签页（DrawMemoryTab）添加：**

```cpp
// Menu.cpp - DrawMemoryTab() 函数中
static void DrawMemoryTab()
{
    ImGui::TextUnformatted("内存功能 - Memory");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::SeparatorText("角色修改 - Player");

    // === 现有控件 ===
    ImGui::Checkbox("移动加速", &MenuSwitches::g_Toggles.PreviewSpeedHack);
    
    // === 添加你的复选框 ===
    ImGui::Checkbox("无限跳跃", &MenuSwitches::g_Toggles.InfiniteJump);
    
    // 可选：添加说明文字
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("开启后可以无限次跳跃，无需落地");
    }
}
```

**ImGui::Checkbox 参数说明：**
```cpp
ImGui::Checkbox("显示文本", &变量引用);
//               ↑           ↑
//            UI上显示的    指向bool变量的指针
//            中文名称      （必须用&取地址）
```

#### 步骤4：在 ConfigSchema.h 中添加配置项

打开 `src/Config/ConfigSchema.h`，找到配置宏定义：

```cpp
// ConfigSchema.h
#define CFG_BOOL_ITEMS(X) \
    X(OpenAimbot) \
    X(BoneOn) \
    /* ... 其他项 */ \
    X(InfiniteJump)  /* 添加你的配置项 */
```

**完成！** 现在编译运行，你的"无限跳跃"复选框就会出现在菜单中，并且支持配置保存/加载。

---

## 添加滑块控件

### 案例：添加"跳跃高度"调节

#### 步骤1：声明变量（MenuSwitches.h）

```cpp
struct ToggleState
{
    // ... 现有变量
    bool InfiniteJump;
    float JumpHeight;         // 跳跃高度倍率
};
```

#### 步骤2：初始化（MenuSwitches.cpp）

```cpp
void ResetDefaults()
{
    // ... 现有初始化
    g_Toggles.InfiniteJump = false;
    g_Toggles.JumpHeight = 1.0f;  // 默认1倍高度
}
```

#### 步骤3：添加滑块（Menu.cpp）

```cpp
// Menu.cpp - DrawMemoryTab() 函数中
ImGui::Checkbox("无限跳跃", &MenuSwitches::g_Toggles.InfiniteJump);

// === 添加滑块 ===
ImGui::SliderFloat("跳跃高度", &MenuSwitches::g_Toggles.JumpHeight, 1.0f, 5.0f, "%.1f 倍");
//                  ↑           ↑                                    ↑      ↑      ↑
//               显示文本      变量引用                            最小值  最大值  格式化
```

**ImGui::SliderFloat 参数详解：**
```cpp
ImGui::SliderFloat(
    "显示文本",              // UI上显示的标签
    &变量引用,               // float* 类型指针
    最小值,                  // float 最小值
    最大值,                  // float 最大值
    "格式化字符串"           // 显示格式，如 "%.2f" "%.0f米" "%.1f倍"
);
```

**常用格式化字符串：**
- `"%.0f"` - 整数显示（如 5）
- `"%.1f"` - 一位小数（如 5.2）
- `"%.2f"` - 两位小数（如 5.23）
- `"%.1f 米"` - 带单位（如 5.2 米）
- `"%.0f%%"` - 百分比（如 50%）

#### 步骤4：添加配置项（ConfigSchema.h）

```cpp
#define CFG_FLOAT_ITEMS(X) \
    X(Aim_Range) \
    X(threshold) \
    /* ... 其他项 */ \
    X(JumpHeight)  /* 添加浮点数配置 */
```

---

## 添加颜色选择器

### 案例：添加"准星颜色"设置

#### 步骤1：声明颜色数组（MenuSwitches.h）

```cpp
struct ToggleState
{
    // ... 现有变量
    
    // === 颜色数组（RGBA，4个float） ===
    float CrosshairColor[4];  // 准星颜色
};
```

**颜色数组说明：**
- `[0]` = Red（红色，0.0-1.0）
- `[1]` = Green（绿色，0.0-1.0）
- `[2]` = Blue（蓝色，0.0-1.0）
- `[3]` = Alpha（透明度，0.0-1.0）

#### 步骤2：初始化颜色（MenuSwitches.cpp）

```cpp
void ResetDefaults()
{
    // ... 现有初始化
    
    // === 初始化为红色，完全不透明 ===
    g_Toggles.CrosshairColor[0] = 1.0f;  // R = 100%
    g_Toggles.CrosshairColor[1] = 0.0f;  // G = 0%
    g_Toggles.CrosshairColor[2] = 0.0f;  // B = 0%
    g_Toggles.CrosshairColor[3] = 1.0f;  // A = 100%
}
```

**常用颜色预设：**
```cpp
// 红色
{1.0f, 0.0f, 0.0f, 1.0f}

// 绿色
{0.0f, 1.0f, 0.0f, 1.0f}

// 蓝色
{0.0f, 0.0f, 1.0f, 1.0f}

// 黄色
{1.0f, 1.0f, 0.0f, 1.0f}

// 紫色
{1.0f, 0.0f, 1.0f, 1.0f}

// 青色
{0.0f, 1.0f, 1.0f, 1.0f}

// 白色
{1.0f, 1.0f, 1.0f, 1.0f}

// 黑色
{0.0f, 0.0f, 0.0f, 1.0f}
```

#### 步骤3：添加颜色选择器（Menu.cpp）

在"颜色"标签页（DrawOverviewTab）添加：

```cpp
// Menu.cpp - DrawOverviewTab() 函数中
void DrawOverviewTab()
{
    ImGui::TextUnformatted("颜色配置 - Colors");
    ImGui::Separator();
    ImGui::Spacing();

    // === 定义颜色选择器标志 ===
    const ImGuiColorEditFlags picker_flags = 
        ImGuiColorEditFlags_NoSidePreview |  // 不显示侧边预览
        ImGuiColorEditFlags_AlphaBar |       // 显示透明度滑块
        ImGuiColorEditFlags_NoInputs |       // 不显示RGB数值输入框
        ImGuiColorEditFlags_AlphaPreview;    // 预览包含透明度

    ImGui::SeparatorText("准星设置");

    // === 添加颜色选择器 ===
    ImGui::ColorEdit4("准星颜色", MenuSwitches::g_Toggles.CrosshairColor, picker_flags);
    //                 ↑           ↑                                        ↑
    //              显示文本      float[4]数组                           选择器样式标志
}
```

**ImGui::ColorEdit4 参数说明：**
```cpp
ImGui::ColorEdit4(
    "显示文本",              // UI上显示的标签
    颜色数组,                // float[4] 数组（RGBA）
    标志位                   // ImGuiColorEditFlags 组合
);
```

**常用标志位组合：**

```cpp
// 完整版（带输入框和侧边预览）
const ImGuiColorEditFlags full_flags = 
    ImGuiColorEditFlags_AlphaBar | 
    ImGuiColorEditFlags_AlphaPreview;

// 简洁版（只有颜色块，点击弹出选择器）
const ImGuiColorEditFlags simple_flags = 
    ImGuiColorEditFlags_NoInputs | 
    ImGuiColorEditFlags_NoLabel;

// 紧凑版（小颜色块）
const ImGuiColorEditFlags compact_flags = 
    ImGuiColorEditFlags_NoInputs | 
    ImGuiColorEditFlags_NoPicker |
    ImGuiColorEditFlags_NoTooltip;
```

#### 步骤4：添加配置项（ConfigSchema.h）

```cpp
#define CFG_COLOR_ITEMS(X) \
    X(ColorBox2D) \
    X(ColorBox3D) \
    /* ... 其他项 */ \
    X(CrosshairColor)  /* 添加颜色配置 */
```

---

## 添加下拉框

### 案例：添加"自瞄骨骼选择"

#### 步骤1：声明变量（MenuSwitches.h）

```cpp
struct ToggleState
{
    // ... 现有变量
    int AimBoneIndex;         // 自瞄骨骼索引（0=头部，1=颈部，2=胸部）
};
```

#### 步骤2：初始化（MenuSwitches.cpp）

```cpp
void ResetDefaults()
{
    // ... 现有初始化
    g_Toggles.AimBoneIndex = 0;  // 默认选择头部
}
```

#### 步骤3：添加下拉框（Menu.cpp）

```cpp
// Menu.cpp - DrawAimbotTab() 函数中
ImGui::Checkbox("启用自瞄", &MenuSwitches::g_Toggles.OpenAimbot);
ImGui::Checkbox("骨骼瞄准", &MenuSwitches::g_Toggles.BoneOn);

// === 添加下拉框 ===
const char* bone_options[] = { "头部", "颈部", "胸部", "腹部" };
ImGui::Combo("瞄准部位", &MenuSwitches::g_Toggles.AimBoneIndex, bone_options, IM_ARRAYSIZE(bone_options));
//            ↑           ↑                                      ↑               ↑
//         显示文本      int*变量                              选项数组        数组大小
```

**ImGui::Combo 参数详解：**
```cpp
ImGui::Combo(
    "显示文本",              // UI上显示的标签
    &变量引用,               // int* 类型指针（存储选中的索引）
    选项数组,                // const char*[] 字符串数组
    数组大小                 // 数组元素个数
);
```

**下拉框使用技巧：**

```cpp
// 方式1：使用字符串数组（推荐）
const char* items[] = { "选项1", "选项2", "选项3" };
ImGui::Combo("标签", &index, items, IM_ARRAYSIZE(items));

// 方式2：使用\0分隔的字符串
ImGui::Combo("标签", &index, "选项1\0选项2\0选项3\0");

// 方式3：带预览文本
const char* items[] = { "低", "中", "高", "超高" };
const char* preview = items[index];  // 当前选中项
if (ImGui::BeginCombo("画质", preview))
{
    for (int n = 0; n < IM_ARRAYSIZE(items); n++)
    {
        bool is_selected = (index == n);
        if (ImGui::Selectable(items[n], is_selected))
            index = n;
        if (is_selected)
            ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
}
```

#### 步骤4：添加配置项（ConfigSchema.h）

```cpp
#define CFG_INT_ITEMS(X) \
    X(ThemeIndex) \
    /* ... 其他项 */ \
    X(AimBoneIndex)  /* 添加整数配置 */
```

---

## 添加按钮

### 案例：添加"一键重置自瞄"按钮

#### 步骤：在 Menu.cpp 中添加按钮

```cpp
// Menu.cpp - DrawAimbotTab() 函数中
ImGui::SliderFloat("自瞄范围", &MenuSwitches::g_Toggles.Aim_Range, 1.0f, 500.0f);
ImGui::SliderFloat("平滑系数", &MenuSwitches::g_Toggles.AimSmooth, 1.0f, 20.0f);

ImGui::Spacing();

// === 添加按钮 ===
if (ImGui::Button("重置自瞄参数", ImVec2(200, 30)))
{
    // 按钮被点击时执行的代码
    MenuSwitches::g_Toggles.Aim_Range = 200.0f;
    MenuSwitches::g_Toggles.AimSmooth = 5.0f;
    MenuSwitches::g_Toggles.AimBoneIndex = 0;
    
    // 可选：显示提示
    ShowToast("自瞄参数已重置", ImVec4(0.45f, 0.95f, 0.55f, 1.0f));
}
```

**ImGui::Button 参数说明：**
```cpp
if (ImGui::Button("按钮文本", ImVec2(宽度, 高度)))
{
    // 按钮被点击时执行
}
```

**按钮样式示例：**

```cpp
// 1. 普通按钮（自动宽度）
if (ImGui::Button("点击我"))
{
    // 执行操作
}

// 2. 固定宽度按钮
if (ImGui::Button("保存配置", ImVec2(150, 0)))  // 宽150，高度自动
{
    Config::SaveConfig("settings.json");
}

// 3. 全宽按钮
ImGuiStyle* s = &ImGui::GetStyle();
if (ImGui::Button("全宽按钮", ImVec2(ImGui::GetContentRegionMax().x - s->WindowPadding.x, 30)))
{
    // 执行操作
}

// 4. 同一行多个按钮
if (ImGui::Button("按钮1", ImVec2(100, 0)))
{
    // 操作1
}
ImGui::SameLine();  // 关键：让下一个控件在同一行
if (ImGui::Button("按钮2", ImVec2(100, 0)))
{
    // 操作2
}

// 5. 禁用按钮
ImGui::BeginDisabled(true);  // 开始禁用
ImGui::Button("禁用按钮");
ImGui::EndDisabled();        // 结束禁用

// 6. 带颜色的按钮
ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));  // 红色
if (ImGui::Button("危险操作"))
{
    // 执行危险操作
}
ImGui::PopStyleColor();
```

---

## 创建新的标签页

### 案例：添加"武器"标签页

#### 步骤1：修改标签页数组（Menu.cpp - Render函数）

找到 `Render()` 函数中的标签页定义：

```cpp
// Menu.cpp - Render() 函数中
const char* nametab_array1[6] = { "E", "D", "A", "G", "C", "B" };
const char* nametab_array[6] = { "瞄准", "视觉", "颜色", "内存", "设置", "配置" };
const char* nametab_hint_array[6] = { 
    "Aim, Recoil, Trigger", 
    "Overlay, Chams, World", 
    "Game Colors", 
    "World Memory", 
    "Element Setup", 
    "Save Settings" 
};
```

**修改为：**

```cpp
// === 将数组大小从 [6] 改为 [7] ===
const char* nametab_array1[7] = { "E", "D", "A", "G", "C", "B", "W" };  // 添加 "W"
const char* nametab_array[7] = { "瞄准", "视觉", "颜色", "内存", "设置", "配置", "武器" };  // 添加 "武器"
const char* nametab_hint_array[7] = { 
    "Aim, Recoil, Trigger", 
    "Overlay, Chams, World", 
    "Game Colors", 
    "World Memory", 
    "Element Setup", 
    "Save Settings",
    "Weapon Mods"  // 添加提示文本
};
```

#### 步骤2：修改标签页循环

找到标签页按钮生成循环：

```cpp
// 原代码
for (int i = 0; i < 6; i++)
    if (ImGui::Tab(...)) tabs = i;
```

**修改为：**

```cpp
// 修改循环次数
for (int i = 0; i < 7; i++)  // 从 6 改为 7
    if (ImGui::Tab(i == tabs, nametab_array1[i], nametab_array[i], nametab_hint_array[i], ImVec2(35 + tab_size, 35))) 
        tabs = i;
```

#### 步骤3：创建标签页函数（Menu.cpp）

在 `Menu.cpp` 文件中添加新函数：

```cpp
// Menu.cpp - 在其他 DrawXXXTab 函数附近添加
static void DrawWeaponTab()
{
    ImGui::TextUnformatted("武器功能 - Weapon");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::SeparatorText("后坐力控制");
    
    ImGui::Checkbox("无后坐力", &MenuSwitches::g_Toggles.PreviewNoRecoil);
    ImGui::SliderFloat("后坐力倍率", &MenuSwitches::g_Toggles.RecoilScale, 0.0f, 1.0f, "%.2f");
    
    ImGui::Spacing();
    ImGui::SeparatorText("射速控制");
    
    ImGui::Checkbox("快速射击", &MenuSwitches::g_Toggles.PreviewRapidFire);
    ImGui::SliderFloat("射速倍率", &MenuSwitches::g_Toggles.FireRateScale, 1.0f, 5.0f, "%.1f 倍");
    
    ImGui::Spacing();
    ImGui::SeparatorText("弹药控制");
    
    ImGui::Checkbox("无限弹药", &MenuSwitches::g_Toggles.PreviewInfiniteAmmo);
    ImGui::SliderInt("弹夹容量", &MenuSwitches::g_Toggles.PreviewAmmoValue, 30, 999);
}
```

#### 步骤4：在 Render 函数中添加标签页渲染

找到标签页渲染的 `if (tabs == X)` 代码块，添加新标签页：

```cpp
// Menu.cpp - Render() 函数中，在最后一个 if (tabs == 5) 后面添加
if (tabs == 6)  // 新标签页索引
{
    ImGui::SetCursorPos(ImVec2(60 + tab_size, 60) + (s->ItemSpacing * 2));
    ImGui::BeginGroup();
    {
        ImGui::BeginChild("D", "Main", ImVec2((c::bg::size.x - 60 - s->ItemSpacing.x * 4) + 18, 520));
        {
            DrawWeaponTab();  // 调用你的标签页函数
        }
        ImGui::EndChild();
    }
    ImGui::EndGroup();
}
```

#### 步骤5：添加必要的变量（如果需要）

在 `MenuSwitches.h` 中添加新变量：

```cpp
struct ToggleState
{
    // ... 现有变量
    
    // === 武器功能变量 ===
    float RecoilScale;
    float FireRateScale;
};
```

在 `MenuSwitches.cpp` 中初始化：

```cpp
void ResetDefaults()
{
    // ... 现有初始化
    
    g_Toggles.RecoilScale = 1.0f;
    g_Toggles.FireRateScale = 1.0f;
}
```

---

## 配置系统集成

### 自动配置保存/加载

你的项目使用 `ConfigSchema.h` 宏定义系统，添加新变量后会自动支持配置保存/加载。

#### 配置宏定义位置（ConfigSchema.h）

```cpp
// ConfigSchema.h

// === 布尔值配置 ===
#define CFG_BOOL_ITEMS(X) \
    X(OpenAimbot) \
    X(BoneOn) \
    X(InfiniteJump) \  /* 你添加的 */
    /* ... 更多 */

// === 浮点数配置 ===
#define CFG_FLOAT_ITEMS(X) \
    X(Aim_Range) \
    X(threshold) \
    X(JumpHeight) \  /* 你添加的 */
    /* ... 更多 */

// === 整数配置 ===
#define CFG_INT_ITEMS(X) \
    X(ThemeIndex) \
    X(AimBoneIndex) \  /* 你添加的 */
    /* ... 更多 */

// === 颜色配置 ===
#define CFG_COLOR_ITEMS(X) \
    X(ColorBox2D) \
    X(CrosshairColor) \  /* 你添加的 */
    /* ... 更多 */
```

**添加规则：**
1. 每行末尾必须有 `\` 续行符
2. 最后一项不需要 `\`
3. 使用 `X(变量名)` 格式
4. 变量名必须与 `MenuSwitches.h` 中的字段名完全一致

---

## 完整案例

### 案例：添加"自定义准星"完整功能

这个案例展示如何从零开始添加一个完整的功能模块。

#### 功能需求
- 开关：启用/禁用自定义准星
- 样式：十字、圆点、方框三种样式
- 大小：可调节准星大小
- 颜色：自定义准星颜色
- 配置：支持保存/加载

---

### 第1步：声明所有变量（MenuSwitches.h）

```cpp
// src/UI/Menu/MenuSwitches.h
struct ToggleState
{
    // ... 现有变量

    // ========== 自定义准星功能 ==========
    bool CustomCrosshair;         // 启用自定义准星
    int CrosshairStyle;           // 准星样式（0=十字，1=圆点，2=方框）
    float CrosshairSize;          // 准星大小
    float CrosshairColor[4];      // 准星颜色（RGBA）
    float CrosshairThickness;     // 准星线条粗细
    bool CrosshairOutline;        // 准星外描边
};
```

---

### 第2步：初始化变量（MenuSwitches.cpp）

```cpp
// src/UI/Menu/MenuSwitches.cpp
void ResetDefaults()
{
    // ... 现有初始化

    // ========== 自定义准星默认值 ==========
    g_Toggles.CustomCrosshair = false;
    g_Toggles.CrosshairStyle = 0;                    // 默认十字样式
    g_Toggles.CrosshairSize = 10.0f;                 // 默认大小10像素
    g_Toggles.CrosshairColor[0] = 0.0f;              // R = 0（绿色准星）
    g_Toggles.CrosshairColor[1] = 1.0f;              // G = 100%
    g_Toggles.CrosshairColor[2] = 0.0f;              // B = 0
    g_Toggles.CrosshairColor[3] = 1.0f;              // A = 100%
    g_Toggles.CrosshairThickness = 2.0f;             // 默认2像素粗细
    g_Toggles.CrosshairOutline = true;               // 默认开启描边
}
```

---

### 第3步：创建UI面板（Menu.cpp）

在 `Menu.cpp` 中创建专门的准星设置函数：

```cpp
// src/UI/Menu/Menu.cpp

// ================================================================
// 函数：DrawCros
继续完整案例的剩余部分：

```markdown
shairPane —— 准星设置面板
// ================================================================
static void DrawCrosshairPane()
{
    ImGui::TextUnformatted("自定义准星 - Crosshair");
    ImGui::Separator();
    ImGui::Spacing();

    // === 主开关 ===
    ImGui::Checkbox("启用自定义准星", &MenuSwitches::g_Toggles.CustomCrosshair);
    
    if (!MenuSwitches::g_Toggles.CustomCrosshair)
    {
        ImGui::TextDisabled("请先启用自定义准星");
        return;
    }

    ImGui::Spacing();
    ImGui::SeparatorText("准星样式");

    // === 样式选择 ===
    const char* styles[] = { "十字", "圆点", "方框" };
    ImGui::Combo("样式", &MenuSwitches::g_Toggles.CrosshairStyle, styles, IM_ARRAYSIZE(styles));

    ImGui::Spacing();
    ImGui::SeparatorText("准星参数");

    // === 大小调节 ===
    ImGui::SliderFloat("大小", &MenuSwitches::g_Toggles.CrosshairSize, 5.0f, 50.0f, "%.0f px");

    // === 粗细调节 ===
    ImGui::SliderFloat("线条粗细", &MenuSwitches::g_Toggles.CrosshairThickness, 1.0f, 5.0f, "%.1f px");

    // === 描边开关 ===
    ImGui::Checkbox("外描边", &MenuSwitches::g_Toggles.CrosshairOutline);
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("在准星外围添加黑色描边，提高可见度");
    }

    ImGui::Spacing();
    ImGui::SeparatorText("准星颜色");

    // === 颜色选择器 ===
    const ImGuiColorEditFlags picker_flags = 
        ImGuiColorEditFlags_NoSidePreview | 
        ImGuiColorEditFlags_AlphaBar | 
        ImGuiColorEditFlags_NoInputs | 
        ImGuiColorEditFlags_AlphaPreview;

    ImGui::ColorEdit4("颜色", MenuSwitches::g_Toggles.CrosshairColor, picker_flags);

    ImGui::Spacing();

    // === 快速颜色预设 ===
    ImGui::Text("快速预设：");
    ImGui::SameLine();
    
    if (ImGui::SmallButton("绿色"))
    {
        MenuSwitches::g_Toggles.CrosshairColor[0] = 0.0f;
        MenuSwitches::g_Toggles.CrosshairColor[1] = 1.0f;
        MenuSwitches::g_Toggles.CrosshairColor[2] = 0.0f;
        MenuSwitches::g_Toggles.CrosshairColor[3] = 1.0f;
    }
    ImGui::SameLine();
    
    if (ImGui::SmallButton("红色"))
    {
        MenuSwitches::g_Toggles.CrosshairColor[0] = 1.0f;
        MenuSwitches::g_Toggles.CrosshairColor[1] = 0.0f;
        MenuSwitches::g_Toggles.CrosshairColor[2] = 0.0f;
        MenuSwitches::g_Toggles.CrosshairColor[3] = 1.0f;
    }
    ImGui::SameLine();
    
    if (ImGui::SmallButton("白色"))
    {
        MenuSwitches::g_Toggles.CrosshairColor[0] = 1.0f;
        MenuSwitches::g_Toggles.CrosshairColor[1] = 1.0f;
        MenuSwitches::g_Toggles.CrosshairColor[2] = 0.0f;
        MenuSwitches::g_Toggles.CrosshairColor[3] = 1.0f;
    }

    ImGui::Spacing();

    // === 预览区域 ===
    ImGui::SeparatorText("实时预览");
    
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImVec2(200, 200);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // 绘制预览背景
    draw_list->AddRectFilled(canvas_pos, 
        ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), 
        IM_COL32(40, 40, 40, 255));
    
    // 绘制准星预览
    ImVec2 center = ImVec2(canvas_pos.x + canvas_size.x * 0.5f, canvas_pos.y + canvas_size.y * 0.5f);
    ImU32 color = ImGui::ColorConvertFloat4ToU32(
        ImVec4(MenuSwitches::g_Toggles.CrosshairColor[0],
               MenuSwitches::g_Toggles.CrosshairColor[1],
               MenuSwitches::g_Toggles.CrosshairColor[2],
               MenuSwitches::g_Toggles.CrosshairColor[3]));
    
    float size = MenuSwitches::g_Toggles.CrosshairSize;
    float thickness = MenuSwitches::g_Toggles.CrosshairThickness;
    
    // 根据样式绘制
    if (MenuSwitches::g_Toggles.CrosshairStyle == 0)  // 十字
    {
        if (MenuSwitches::g_Toggles.CrosshairOutline)
        {
            // 黑色描边
            draw_list->AddLine(ImVec2(center.x - size, center.y), ImVec2(center.x + size, center.y), 
                IM_COL32(0, 0, 0, 255), thickness + 2.0f);
            draw_list->AddLine(ImVec2(center.x, center.y - size), ImVec2(center.x, center.y + size), 
                IM_COL32(0, 0, 0, 255), thickness + 2.0f);
        }
        // 主准星
        draw_list->AddLine(ImVec2(center.x - size, center.y), ImVec2(center.x + size, center.y), 
            color, thickness);
        draw_list->AddLine(ImVec2(center.x, center.y - size), ImVec2(center.x, center.y + size), 
            color, thickness);
    }
    else if (MenuSwitches::g_Toggles.CrosshairStyle == 1)  // 圆点
    {
        if (MenuSwitches::g_Toggles.CrosshairOutline)
        {
            draw_list->AddCircleFilled(center, size * 0.5f + 1.0f, IM_COL32(0, 0, 0, 255));
        }
        draw_list->AddCircleFilled(center, size * 0.5f, color);
    }
    else if (MenuSwitches::g_Toggles.CrosshairStyle == 2)  // 方框
    {
        if (MenuSwitches::g_Toggles.CrosshairOutline)
        {
            draw_list->AddRect(
                ImVec2(center.x - size, center.y - size),
                ImVec2(center.x + size, center.y + size),
                IM_COL32(0, 0, 0, 255), 0.0f, 0, thickness + 2.0f);
        }
        draw_list->AddRect(
            ImVec2(center.x - size, center.y - size),
            ImVec2(center.x + size, center.y + size),
            color, 0.0f, 0, thickness);
    }
    
    ImGui::Dummy(canvas_size);  // 占位
}
```

---

### 第4步：将面板添加到标签页

选择一个合适的标签页添加准星设置。例如在"设置"标签页：

```cpp
// src/UI/Menu/Menu.cpp - DrawSettingsTab() 函数中
void DrawSettingsTab()
{
    ImGui::TextUnformatted("设置 - Element Setup");
    ImGui::Separator();
    ImGui::Spacing();

    // ... 现有设置项

    ImGui::Spacing();
    ImGui::SeparatorText("准星设置");
    
    // === 调用准星面板 ===
    DrawCrosshairPane();
}
```

或者创建独立的子窗口：

```cpp
// 在标签页中创建子窗口
if (tabs == 4)  // 设置页
{
    ImGui::SetCursorPos(ImVec2(60 + tab_size, 60) + (s->ItemSpacing * 2));
    
    // 左侧：常规设置
    ImGui::BeginGroup();
    {
        ImGui::BeginChild("Settings", "常规设置", ImVec2(300, 500));
        {
            // 常规设置内容
        }
        ImGui::EndChild();
    }
    ImGui::EndGroup();
    
    ImGui::SameLine();
    
    // 右侧：准星设置
    ImGui::BeginGroup();
    {
        ImGui::BeginChild("Crosshair", "准星设置", ImVec2(320, 500));
        {
            DrawCrosshairPane();  // 准星面板
        }
        ImGui::EndChild();
    }
    ImGui::EndGroup();
}
```

---

### 第5步：添加配置项（ConfigSchema.h）

```cpp
// src/Config/ConfigSchema.h

// === 布尔值配置 ===
#define CFG_BOOL_ITEMS(X) \
    X(OpenAimbot) \
    X(BoneOn) \
    /* ... 其他项 */ \
    X(CustomCrosshair) \
    X(CrosshairOutline)

// === 整数配置 ===
#define CFG_INT_ITEMS(X) \
    X(ThemeIndex) \
    /* ... 其他项 */ \
    X(CrosshairStyle)

// === 浮点数配置 ===
#define CFG_FLOAT_ITEMS(X) \
    X(Aim_Range) \
    X(threshold) \
    /* ... 其他项 */ \
    X(CrosshairSize) \
    X(CrosshairThickness)

// === 颜色配置 ===
#define CFG_COLOR_ITEMS(X) \
    X(ColorBox2D) \
    X(ColorBox3D) \
    /* ... 其他项 */ \
    X(CrosshairColor)
```

---

### 第6步：实现准星绘制（可选）

如果要在游戏中实际绘制准星，在渲染循环中添加：

```cpp
// src/UI/Draw/Draw.cpp 或 DX11.cpp 的渲染函数中

void RenderCustomCrosshair()
{
    if (!MenuSwitches::g_Toggles.CustomCrosshair)
        return;

    ImDrawList* draw_list = ImGui::GetForegroundDrawList();
    ImVec2 screen_center = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, 
                                   ImGui::GetIO().DisplaySize.y * 0.5f);
    
    ImU32 color = ImGui::ColorConvertFloat4ToU32(
        ImVec4(MenuSwitches::g_Toggles.CrosshairColor[0],
               MenuSwitches::g_Toggles.CrosshairColor[1],
               MenuSwitches::g_Toggles.CrosshairColor[2],
               MenuSwitches::g_Toggles.CrosshairColor[3]));
    
    float size = MenuSwitches::g_Toggles.CrosshairSize;
    float thickness = MenuSwitches::g_Toggles.CrosshairThickness;
    
    // 绘制描边
    if (MenuSwitches::g_Toggles.CrosshairOutline)
    {
        if (MenuSwitches::g_Toggles.CrosshairStyle == 0)  // 十字描边
        {
            draw_list->AddLine(
                ImVec2(screen_center.x - size, screen_center.y), 
                ImVec2(screen_center.x + size, screen_center.y), 
                IM_COL32(0, 0, 0, 255), thickness + 2.0f);
            draw_list->AddLine(
                ImVec2(screen_center.x, screen_center.y - size), 
                ImVec2(screen_center.x, screen_center.y + size), 
                IM_COL32(0, 0, 0, 255), thickness + 2.0f);
        }
        else if (MenuSwitches::g_Toggles.CrosshairStyle == 1)  // 圆点描边
        {
            draw_list->AddCircleFilled(screen_center, size * 0.5f + 1.0f, IM_COL32(0, 0, 0, 255));
        }
        else if (MenuSwitches::g_Toggles.CrosshairStyle == 2)  // 方框描边
        {
            draw_list->AddRect(
                ImVec2(screen_center.x - size, screen_center.y - size),
                ImVec2(screen_center.x + size, screen_center.y + size),
                IM_COL32(0, 0, 0, 255), 0.0f, 0, thickness + 2.0f);
        }
    }
    
    // 绘制主准星
    if (MenuSwitches::g_Toggles.CrosshairStyle == 0)  // 十字
    {
        draw_list->AddLine(
            ImVec2(screen_center.x - size, screen_center.y), 
            ImVec2(screen_center.x + size, screen_center.y), 
            color, thickness);
        draw_list->AddLine(
            ImVec2(screen_center.x, screen_center.y - size), 
            ImVec2(screen_center.x, screen_center.y + size), 
            color, thickness);
    }
    else if (MenuSwitches::g_Toggles.CrosshairStyle == 1)  // 圆点
    {
        draw_list->AddCircleFilled(screen_center, size * 0.5f, color);
    }
    else if (MenuSwitches::g_Toggles.CrosshairStyle == 2)  // 方框
    {
        draw_list->AddRect(
            ImVec2(screen_center.x - size, screen_center.y - size),
            ImVec2(screen_center.x + size, screen_center.y + size),
            color, 0.0f, 0, thickness);
    }
}

// 在主渲染循环中调用
void Render()
{
    // ... ImGui 渲染代码
    
    RenderCustomCrosshair();  // 绘制准星
    
    // ... 其他渲染
}
```

---

### 第7步：编译测试

```bash
# 使用 MSBuild 编译
msbuild ShootHack.sln /p:Configuration=Release /p:Platform=x64

# 或使用 Visual Studio
# 打开 ShootHack.sln，按 F7 编译
```

**测试清单：**
- [ ] 菜单中能看到"自定义准星"选项
- [ ] 勾选后能调节各项参数
- [ ] 预览区域正确显示准星样式
- [ ] 保存配置后重启能正确加载
- [ ] 游戏中准星正确显示（如果实现了绘制）

---

## 常见问题排查

### Q1: 添加的控件不显示？

**检查项：**
```cpp
// 1. 变量是否正确声明？
// MenuSwitches.h
bool MyFeature;  // ✓ 正确

// 2. 变量是否初始化？
// MenuSwitches.cpp
g_Toggles.MyFeature = false;  // ✓ 正确

// 3. 是否使用了引用？
ImGui::Checkbox("标签", &MenuSwitches::g_Toggles.MyFeature);  // ✓ 正确（有&）
ImGui::Checkbox("标签", MenuSwitches::g_Toggles.MyFeature);   // ✗ 错误（缺少&）

// 4. 是否在正确的标签页？
if (tabs == 0)  // 确认标签页索引
{
    // 你的控件代码
}
```

### Q2: 配置保存后加载无效？

**检查项：**
```cpp
// 1. 是否添加到 ConfigSchema.h？
#define CFG_BOOL_ITEMS(X) \
    X(MyFeature)  // ✓ 已添加

// 2. 变量名是否完全一致？
// MenuSwitches.h
bool MyFeature;

// ConfigSchema.h
X(MyFeature)  // ✓ 名称一致

// 3. 是否有语法错误？
#define CFG_BOOL_ITEMS(X) \
    X(Feature1) \
    X(Feature2) \
    X(Feature3)  // ✓ 最后一项不需要 \
```

### Q3: 滑块拖动无反应？

**检查项：**
```cpp
// 1. 变量类型是否匹配？
float MyValue;  // 变量是 float
ImGui::SliderFloat("标签", &MyValue, 0.0f, 100.0f);  // ✓ 使用 SliderFloat

int MyValue;  // 变量是 int
ImGui::SliderInt("标签", &MyValue, 0, 100);  // ✓ 使用 SliderInt

// 2. 范围是否合理？
ImGui::SliderFloat("标签", &MyValue, 100.0f, 0.0f);  // ✗ 最小值 > 最大值
ImGui::SliderFloat("标签", &MyValue, 0.0f, 100.0f);  // ✓ 正确

// 3. 是否使用了引用？
ImGui::SliderFloat("标签", &MyValue, 0.0f, 100.0f);  // ✓ 有 &
```

### Q4: 颜色选择器不工作？

**检查项：**
```cpp
// 1. 数组大小是否正确？
float MyColor[4];  // ✓ 必须是 4 个元素（RGBA）
float MyColor[3];  // ✗ 错误

// 2. 是否初始化？
MyColor[0] = 1.0f;  // R
MyColor[1] = 0.0f;  // G
MyColor[2] = 0.0f;  // B
MyColor[3] = 1.0f;  // A

// 3. 是否使用 ColorEdit4？
ImGui::ColorEdit4("颜色", MyColor, flags);  // ✓ 正确
ImGui::ColorEdit3("颜色", MyColor, flags);  // ✗ 只有 RGB，没有 Alpha
```

### Q5: 编译错误？

**常见错误：**
```cpp
// 错误1：未声明的标识符
error C2065: 'MyFeature': 未声明的标识符
// 解决：在 MenuSwitches.h 中声明变量

// 错误2：语法错误
error C2143: 语法错误: 缺少";"
// 解决：检查结构体定义末尾是否有分号

struct ToggleState
{
    bool MyFeature;
};  // ← 这里必须有分号

// 错误3：重定义
error C2086: 'MyFeature': 重定义
// 解决：检查是否重复声明变量

// 错误4：类型不匹配
error C2664: 无法将参数 2 从 'bool' 转换为 'bool *'
// 解决：添加 & 取地址符
ImGui::Checkbox("标签", &MyFeature);  // 正确
```

---

## 高级技巧

### 技巧1：条件显示控件

```cpp
// 只有开启主开关后才显示子选项
ImGui::Checkbox("启用功能", &MenuSwitches::g_Toggles.EnableFeature);

if (MenuSwitches::g_Toggles.EnableFeature)
{
    ImGui::Indent();  // 缩进子选项
    ImGui::SliderFloat("参数1", &MenuSwitches::g_Toggles.Param1, 0.0f, 100.0f);
    ImGui::SliderFloat("参数2", &MenuSwitches::g_Toggles.Param2, 0.0f, 100.0f);
    ImGui::Unindent();
}
else
{
    ImGui::TextDisabled("请先启用功能");
}
```

### 技巧2：分组显示

```cpp
ImGui::SeparatorText("基础设置");
ImGui::Checkbox("选项1", &option1);
ImGui::Checkbox("选项2", &option2);

ImGui::Spacing();
ImGui::SeparatorText("高级设置");
ImGui::SliderFloat("参数1", &param1, 0.0f, 100.0f);
ImGui::SliderFloat("参数2", &param2, 0.0f, 100.0f);
```

### 技巧3：同一行显示多个控件

```cpp
// 方式1：使用 SameLine()
ImGui::Checkbox("选项1", &option1);
ImGui::SameLine();
ImGui::Checkbox("选项2", &option2);

// 方式2：指定间距
ImGui::Checkbox("选项1", &option1);
ImGui::SameLine(0.0f, 20.0f);  // 间距20像素
ImGui::Checkbox("选项2", &option2);

// 方式3：多个按钮
if (ImGui::Button("按钮1", ImVec2(100, 0))) { }
ImGui::SameLine();
if (ImGui::Button("按钮2", ImVec2(100, 0))) { }
ImGui::SameLine();
if (ImGui::Button("按钮3", ImVec2(100, 0)) { }
```

### 技巧4：添加提示文本

```cpp
// 方式1：悬停提示
ImGui::Checkbox("复杂功能", &feature);
ImGui::SameLine();
ImGui::TextDisabled("(?)");
if (ImGui::IsItemHovered())
{
    ImGui::SetTooltip("这是一个详细的功能说明\n可以多行显示");
}

// 方式2：直接说明
ImGui::Checkbox("危险功能", &dangerous);
ImGui::SameLine();
ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "警告：可能导致封号");
```

### 技巧5：禁用控件

```cpp
// 条件禁用
bool canUse = CheckPermission();

ImGui::BeginDisabled(!canUse);
ImGui::Checkbox("需要权限的功能", &feature);
ImGui::EndDisabled();

if (!canUse)
{
    ImGui::SameLine();
    ImGui::TextDisabled("(需要VIP)");
}
```

---

## 总结

### 添加新功能的标准流程

1. **声明变量** → `MenuSwitches.h`
2. **初始化变量** → `MenuSwitches.cpp`
3. **添加UI控件** → `Menu.cpp`
4. **添加配置项** → `ConfigSchema.h`
5. **编译测试** → 验证功能

### 常用控件速查

| 控件类型 | 函数 | 用途 |
|---------|------|------|
| 复选框 | `ImGui::Checkbox()` | 开关功能 |
| 滑块（浮点） | `ImGui::SliderFloat()` | 调节数值 |
| 滑块（整数） | `ImGui::SliderInt()` | 调节整数 |
| 下拉框 | `ImGui::Combo()` | 选择选项 |
| 颜色选择器 | `ImGui::ColorEdit4()` | 选择颜色 |
| 按钮 | `ImGui::Button()` | 触发操作 |
| 文本 | `ImGui::Text()` | 显示文字 |
| 输入框 | `ImGui::InputText()` | 输入文本 |

### 最佳实践

✅ **推荐做法：**
- 使用有意义的变量名
- 添加功能说明和提示
- 合理分组和排版
- 及时添加到配置系统
- 编译前检查语法

❌ **避免做法：**
- 变量名过于简短（如 `a`, `b`, `c`）
- 忘记初始化变量
- 忘记添加 `&` 引用符
- 配置项名称与变量名不一致
- 不测试就提交代码

---

**文档版本：** v1.0  
**最后更新：** 2026-04-27  
**适用项目：** ShootHack UI System  
**作者：** Kiro AI Assistant
```

这份文档涵盖了：
1. ✅ 从零开始的完整流程
2. ✅ 每种控件的详细用法和参数说明
3. ✅ 真实的代码示例（基于你的项目结构）
4. ✅ 完整的案例（自定义准星功能）
5. ✅ 常见问题排查
6. ✅ 高级技巧
7. ✅ 最佳实践建议

你可以直接复制到 `docs/UI实操教程.md` 文件中使用！