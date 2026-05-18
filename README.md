# ShootHack

> 一个基于 DirectX 11 的 Unity 游戏外挂框架，支持 IL2CPP 运行时逆向分析。

## 概述

ShootHack 是一个面向 FPS/Unity 游戏（IL2CPP 运行时）的逆向工程与辅助功能框架。它通过 DX11 挂接（Present Hook）实现渲染覆盖层，利用 MinHook 进行 API Hook，配合 ImGui 构建完整的 UI 交互系统。

## 项目结构

```
ShootHack/
├── src/                    # 核心源代码
│   ├── Core/               # 基础类型、全局变量、结构定义
│   ├── Engine/             # IL2CPP 引擎分析（类/方法/对象）
│   ├── Hook/               # DX11 Hook 实现
│   ├── Config/             # 配置系统
│   ├── UI/
│   │   ├── Menu/           # 菜单与功能开关
│   │   ├── Draw/           # 绘制模块
│   │   ├── Render/         # 渲染相关
│   │   └── Widgets/        # 自定义控件
│   ├── ESP/                # ESP 绘制
│   ├── Assets/             # 资源文件（字体、图片等）
│   └── ThirdParty/         # 第三方头文件（json.hpp）
├── sdk/                    # 游戏 SDK 定义（Il2CppDumper 导出）
├── ImGui/                  # Dear ImGui (魔改版)
├── MinHook/                # MinHook 库
├── freetype/               # FreeType 字体渲染
├── docs/                   # 开发文档
├── Scripts/                # 辅助脚本（Frida、图片转换等）
├── misc/                   # 杂项（额外 freetype 集成参考）
├── ShootHack.vcxproj       # Visual Studio 2022 项目文件
└── ShootHack.sln           # 解决方案文件
```

## 构建要求

- **Visual Studio 2022** (v143 工具集)
- **Windows 10 SDK** (10.0+)
- **C++17** 标准
- **x64** 平台 (仅 64 位)

## 快速开始

1. 打开 `ShootHack.sln`
2. 选择 `x64 | Debug` 或 `x64 | Release` 配置
3. 按 `F7` 生成
4. 生成产物位于 `x64/Debug/ShootHack.dll` (或 `x64/Release/ShootHack.dll`)
5. 使用任意 DLL 注入工具将 `ShootHack.dll` 注入到目标 Unity 游戏进程

## 文档

详细文档请参阅 [`docs/README.md`](docs/README.md)。

## 功能特性

- **DX11 Present Hook** — 基于 MinHook 的 DirectX 11 渲染挂接
- **ImGui 集成** — 完整的 Dear ImGui UI 框架，支持自定义控件、图片、Logo
- **ESP 透视** — 玩家/物体绘制与骨骼渲染
- **IL2CPP 运行时分析** — Unity IL2CPP 对象、类、方法逆向支持
- **配置系统** — JSON 配置文件读写，支持热加载
- **菜单系统** — 功能开关 + 搜索过滤
- **FPS 计数器** — 覆盖层帧率显示
- **图片系统** — 支持内嵌与外部图片加载（自定义 Logo 等）

## 目标游戏

本项目当前针对 **ShootHouse**（一款 Unity IL2CPP FPS 小游戏）进行开发与测试。

---

## 路线图

### v1.1（下一个版本）
- [ ] **GOM 直接内存访问** — 基于 GameObjectManager 的手动遍历，取代 IL2CPP API 高频调用，提升反检测能力
- [ ] **完整人物方框 ESP** — 2D/3D 方框、拐角方框、距离显示、血量条
- [ ] **骨骼绘制** — 基于 Transform 层级结构实现人物骨骼透视
- [ ] **雷达绘制** — 小地图缩略雷达，显示周围敌人方位

### v1.2（下下个版本）
- [ ] **自瞄系统** — 平滑瞄准、骨骼瞄准、FOV 限制、可见性检测

---

## 技术栈

| 组件       | 说明                  |
| ---------- | --------------------- |
| DirectX 11 | 渲染挂接目标          |
| ImGui      | UI 框架 (魔改版)      |
| MinHook    | API Hook 库           |
| FreeType   | 字体渲染              |
| C++17      | 开发语言标准          |
| IL2CPP     | Unity 脚本后端分析    |

## 免责声明

本项目仅供学习和研究使用。请勿将其用于任何违反游戏服务条款的用途。使用本软件产生的任何后果由使用者自行承担。

## 许可

[GNU General Public License v3.0](LICENSE)
