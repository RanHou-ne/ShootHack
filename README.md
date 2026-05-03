# ShootHack

> 一个基于 DirectX 11 的 Unity 游戏外挂框架，支持 IL2CPP 运行时逆向分析。

## 概述

ShootHack 是一个面向 FPS/Unity 游戏（IL2CPP 运行时）的逆向工程与辅助功能框架。它通过 DX11 挂接（Present Hook）实现渲染覆盖层，利用 MinHook 进行 API Hook，配合 ImGui 构建完整的 UI 交互系统。

## 功能特性

- **DX11 Present Hook** — 基于 MinHook 的 DirectX 11 渲染挂接
- **ImGui 集成** — 完整的 Dear ImGui UI 框架，支持自定义控件、图片、Logo
- **ESP 透视** — 玩家/物体绘制与骨骼渲染
- **IL2CPP 运行时分析** — Unity IL2CPP 对象、类、方法逆向支持
- **配置系统** — JSON 配置文件读写，支持热加载
- **菜单系统** — 功能开关 + 搜索过滤
- **FPS 计数器** — 覆盖层帧率显示
- **图片系统** — 支持内嵌与外部图片加载（自定义 Logo 等）

## 项目结构

```
ShootHack/
├── src/                  # 核心源代码
│   ├── Core/             # 基础类型、全局变量、结构定义
│   ├── Engine/           # IL2CPP 引擎分析（类/方法/对象）
│   ├── Hook/             # DX11 Hook 实现
│   ├── Config/           # 配置系统
│   ├── UI/
│   │   ├── Menu/         # 菜单与功能开关
│   │   └── Draw/         # 绘制模块
│   ├── ESP/              # ESP 绘制
│   ├── SDK/              # 游戏 SDK 定义（dump 生成）
│   └── Assets/           # 资源文件
├── ImGui/                # Dear ImGui (魔改版)
├── MinHook/              # MinHook 库
├── freetype/             # FreeType 字体渲染
├── docs/                 # 开发文档
│   ├── README.md         # 文档索引
│   ├── UI实操教程.md     # UI 开发教程
│   ├── IL2CPP_逆向分析笔记.md
│   └── ...               # 更多教程与说明
├── ShootHack.vcxproj     # Visual Studio 2022 项目文件
└── ShootHack.sln         # 解决方案文件
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

[MIT](LICENSE)
