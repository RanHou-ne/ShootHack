# ShootHack 项目结构说明

## 目录结构

```
ShootHack/
├── src/                          ← 所有源代码（新结构，参与编译）
│   ├── Core/                     ← 核心层：基础类型、全局状态
│   │   ├── Base.h                  全局公共头文件（Windows/DX11/STL）
│   │   ├── Types.h                 基础数据类型（Vector2/3/4、颜色、菜单状态）
│   │   ├── globals.h               全局变量声明（extern）
│   │   └── globals.cpp             全局变量定义 + 初始化/清理
│   │
│   ├── Engine/                   ← 引擎层：IL2CPP 运行时交互
│   │   ├── Engine.h                引擎接口 + IL2CPP API 声明（X-Macro）
│   │   ├── Engine.cpp              引擎实现 + IL2CPP API 动态绑定
│   │   └── il2cpp_api.h            IL2CPP C API 列表（X-Macro 宏表）
│   │
│   ├── Hook/                     ← Hook 层：DX11 虚表 Hook
│   │   ├── HookManager.h           Hook 管理器接口
│   │   ├── HookManager.cpp         Hook 管理器实现（统一初始化/卸载）
│   │   ├── PresentHook.h           Present Hook 包装器接口
│   │   ├── PresentHook.cpp         Present Hook 包装器实现
│   │   └── DX11Hook.h              DX11 虚表 Hook 核心接口
│   │
│   ├── Config/                   ← 配置层：JSON 序列化引擎
│   │   ├── Config.h                配置接口（Save/Load/Reset）
│   │   ├── Config.cpp              配置实现（X-Macro 自动遍历）
│   │   └── ConfigSchema.h          配置项宏表（新增配置只改这里）
│   │
│   ├── UI/                       ← UI 层：菜单和绘制
│   │   ├── Menu/
│   │   │   ├── Menu.h              菜单主框架接口
│   │   │   ├── Menu.cpp            菜单主框架实现（Render + 各 Tab）
│   │   │   ├── MenuSwitches.h      菜单状态结构体（所有开关/参数）
│   │   │   ├── MenuSwitches.cpp    菜单状态实例定义
│   │   │   ├── MenuFeatureStubs.h  功能预留接口声明
│   │   │   └── MenuFeatureStubs.cpp 功能预留接口实现（对接游戏逻辑）
│   │   └── Draw/
│   │       ├── Draw.h              绘制工具类接口（DrawUtils + Notification）
│   │       └── Draw.cpp            绘制工具类实现
│   │
│   ├── SDK/                      ← SDK 层：游戏数据结构
│   │   └── struct.h                游戏相关结构体（可扩展）
│   │
│   ├── Assets/                   ← 资源层：字体/图片数据
│   │   └── （字体/图片头文件放这里）
│   │
│   └── dllmain.cpp               ← DLL 入口点
│
├── ImGui/                        ← ImGui 库（第三方，不修改）
│   ├── imgui.h / imgui.cpp
│   ├── imgui_impl_dx11.h/cpp     DX11 渲染后端
│   ├── imgui_impl_win32.h/cpp    Win32 输入后端
│   ├── imgui_settings.h          自定义样式常量（可修改）
│   └── ...
│
├── sdk/                          ← 游戏 SDK 参考（不参与编译）
│   ├── dump.cs                   IL2CPP dump 文件
│   ├── il2cpp.h                  IL2CPP 类型定义
│   └── script.json               类/方法/字段偏移表
│
├── mofang/                       ← 旧版本代码（参考用，不编译）
├── legacy/                       ← 遗留代码（不编译）
├── docs/                         ← 文档
│   └── PROJECT_STRUCTURE.md      本文件
│
├── ShootHack.vcxproj             ← Visual Studio 项目文件
└── ShootHack.sln                 ← Visual Studio 解决方案文件
```

---

## 模块依赖关系

```
dllmain.cpp
    ↓
Engine::Initialize()          绑定 IL2CPP API
Hook::InitializeAllHooks()    安装 DX11 虚表 Hook
    ↓
PresentHook::Initialize()
    ↓
DX11HOOK()                    修改 SwapChain 虚表
    ↓
hkPresent() [每帧]
    ├── ImGui 初始化（首次）
    │   ├── 加载字体（Assets/）
    │   ├── 加载纹理（Assets/）
    │   ├── Config::Initialize()
    │   └── Config::Load()（如启用自动加载）
    │
    └── Menu::Render() [每帧]
        ├── DrawAimbotTab()    → MenuSwitches::g_Toggles
        ├── DrawESPTab()       → MenuSwitches::g_Toggles
        ├── DrawOverviewTab()  → MenuSwitches::g_Toggles
        ├── DrawMemoryTab()    → MenuSwitches::g_Toggles
        ├── DrawSettingsTab()  → MenuSwitches::g_Toggles
        └── DrawConfigTab()    → Config::Save/Load/Reset
```

---

## 扩展指南

### 添加新的菜单功能开关

1. **MenuSwitches.h** — 在 `ToggleState` 中添加字段：
   ```cpp
   bool MyNewFeature = false;  // 新功能开关
   ```

2. **ConfigSchema.h** — 在 `CFG_BOOL_ITEMS` 中添加宏：
   ```cpp
   CFG_BOOL("MyNewFeature", MyNewFeature, false, "新功能")
   ```

3. **Menu.cpp** — 在对应 Tab 中添加 ImGui 控件：
   ```cpp
   ImGui::Checkbox("新功能", &MenuSwitches::g_Toggles.MyNewFeature);
   ```

4. **MenuFeatureStubs.cpp** — 实现对应逻辑（可选）

### 添加新的 Tab 页面

1. 在 `Menu.h` 中声明 `void DrawMyTab();`
2. 在 `Menu.cpp` 中实现函数体
3. 在 `Render()` 的 Tab 数组中添加新 Tab 名称（将数组大小从 6 改为 7）
4. 在 `Render()` 中添加 `if (tabs == 6) { DrawMyTab(); }` 分支

### 添加新的 IL2CPP API

在 `src/Engine/il2cpp_api.h` 中添加一行：
```cpp
DO_API(ReturnType, function_name, (ArgType1 arg1, ArgType2 arg2))
```
重新编译即可，声明/定义/加载全部自动生成。

### 添加新的颜色配置

1. **MenuSwitches.h** — 添加颜色字段：
   ```cpp
   float ColorMyElement[4] = {1.0f, 0.0f, 0.0f, 1.0f};
   ```

2. **ConfigSchema.h** — 在 `CFG_COLOR_ITEMS` 中添加（注意用独立 float，不用花括号）：
   ```cpp
   CFG_COLOR("ColorMyElement", ColorMyElement, 1.0f, 0.0f, 0.0f, 1.0f, "我的元素颜色")
   ```

3. **Menu.cpp** — 在颜色页添加控件：
   ```cpp
   ImGui::ColorEdit4("我的元素颜色", MenuSwitches::g_Toggles.ColorMyElement, picker_flags);
   ```

---

## 关键设计模式

| 模式 | 应用场景 | 优势 |
|------|---------|------|
| **X-Macro** | ConfigSchema + IL2CPP API | 集中管理，避免重复代码 |
| **虚表 Hook** | DX11 Present/Resize 拦截 | 无需 Detour，直接改写虚表 |
| **重入保护** | hkPresent 递归调用防护 | 防止栈溢出崩溃 |
| **延迟初始化** | ImGui 在首次 Present 时初始化 | 确保 DX11 设备已就绪 |
| **预留接口** | MenuFeatureStubs | UI 与功能实现解耦 |
| **全局状态集中** | MenuSwitches::g_Toggles | 单一数据源，便于序列化 |
| **RAII 线程安全** | Engine::ThreadGuard | 自动 attach/detach，防止 GC 崩溃 |
| **链式查找封装** | Engine::GetClass / GetTypeObject | 消除 domain→assembly→image→class 重复代码 |

---

## 封装工具速查

### Engine::ThreadGuard — IL2CPP 线程安全

```cpp
// 在渲染线程调用任何 IL2CPP API 前，必须 attach 当前线程
// 使用 RAII 封装，离开作用域自动 detach

void MyESPFunction()
{
    Engine::ThreadGuard guard;        // 构造时自动 attach
    if (!guard.IsValid()) return;     // attach 失败

    // ... 调用 IL2CPP API，可以自由 return ...
}                                    // 离开作用域，自动 detach
```

### Engine::GetClass — 获取 Il2CppClass*

```cpp
// 一行获取任意类的元信息指针
// 内部封装: domain → assembly → image → class

Il2CppClass* pClass = Engine::GetClass("UnityEngine.CoreModule", "UnityEngine", "GameObject");
if (!pClass) return;

// 用途：字段遍历、方法查找、DumpClass 等
```

### Engine::GetTypeObject — 获取 C# System.Type 实例

```cpp
// 一行获取 System.Type 实例，给 FindObjectsOfType 等需要 Type 参数的方法用
// 内部封装: GetClass → class_get_type → type_get_object

Il2CppObject* pTypeObj = Engine::GetTypeObject("Assembly-CSharp", "GameSpecific", "Player");
if (!pTypeObj) return;

auto* arr = (System_Object_array*)FindObjectsOfType(pTypeObj);
// 遍历 arr...
```
