# GameObjectManager (GOM) 偏移查找完整指南

## 📌 概述

本指南帮你在 `dump.cs` 中找到 GameObjectManager 及其关键字段的偏移，用于 Frida 脚本的内存访问。

---

## 🔍 第一步：确认是否有 GameObjectManager 导出

### 命令行查找
```powershell
# 在 dump.cs 中搜索 GameObjectManager
Select-String -Path dump.cs -Pattern "GameObjectManager" -ErrorAction SilentlyContinue

# 或搜索常见的 GOM 别名
Select-String -Path dump.cs -Pattern "s_Instance|ActiveObjects|gameObjects"
```

### 预期结果
- ✅ **有定义**：会看到类似 `internal static GameObjectManager s_Instance; // 0xABCDEF00`
- ❌ **无定义**：此游戏未导出 GameObjectManager，需使用备选方案（见下文）

---

## 🔎 第二步：Tag 字段偏移 (TAG_OFFSET)

### 在 dump.cs 中找到 GameObject 类

```
搜索关键词：public sealed class GameObject : Object
```

### 定位 tag 字段

在 GameObject 类定义中，找到：
```csharp
public string tag { get; }  // 0x5C
```

**记录下注释中的偏移值（通常在 // 后面）**

### 示例截图位置
```
public sealed class GameObject : Object // TypeDefIndex: 2764
{
    // Properties
    public string tag { get; }              // ← 在这里找 0xXX
```

**写入 Frida 脚本：**
```javascript
const TAG_OFFSET = 0x5C;  // 替换为你找到的值
```

---

## 🔎 第三步：Transform 指针偏移 (TRANSFORM_OFFSET)

### 在 dump.cs 中继续查找 GameObject 类

在同一个 GameObject 类中，找到：
```csharp
public Transform transform { get; }  // 0x28
```

**记录这个偏移值**

### 示例
```
public sealed class GameObject : Object // TypeDefIndex: 2764
{
    // Properties
    public Transform transform { get; }  // ← 0x28
    public int layer { get; set; }
    public string tag { get; }           // ← 0x5C
```

**写入 Frida 脚本：**
```javascript
const TRANSFORM_OFFSET = 0x28;  // 替换为你找到的值
```

---

## 🔎 第四步：Position 偏移 (POS_OFFSET)

### 在 dump.cs 中找到 Transform 类

```
搜索关键词：public class Transform : Component
```

### 定位 position 字段

在 Transform 类中，找到：
```csharp
public Vector3 position { get; set; }  // 0x30
```

**通常这个值是固定的 0x30，但请确认！**

**写入 Frida 脚本：**
```javascript
const POS_OFFSET = 0x30;  // 替换为你找到的值
```

---

## 🔴 第五步：GOM_OFFSET（最关键！）

### 情况 A：游戏导出了 GameObjectManager

在 dump.cs 中搜索：
```
private static GameObjectManager s_Instance;  // 0xABCDEF00
```

记下这个偏移值，**但这通常是符号表地址，不能直接用**。

### 情况 B：游戏未导出 GameObjectManager（本游戏情况）

**此时需要使用 CheatEngine 动态查找：**

#### 步骤：
1. **注入 Frida + CheatEngine**
   ```bash
   frida -U -f ShootHack.exe -l GOM_Frida_Script.js
   ```

2. **在 CheatEngine 中附加进程**
   - 选择 `ShootHack` 进程
   - 确保主要的 GameObject（如 Player）已加载到内存

3. **查找 Player GameObject 的地址**
   - 搜索字符串 "Player"（你知道这是个 GameObject 的 tag）
   - 或直接用 Frida 脚本临时打印地址

4. **反推 GOM 的地址**
   - 如果找到 activeObjects vector：
     ```
     playerAddr = 0x4A000000
     vector 起始地址 = 0x4B000000
     GOM_Address ≈ 0x4B000000 - 0x10 (ACTIVE_OFFSET)
                = 0x4AFF0000
     GOM_OFFSET = 0x4AFF0000 - GameAssembly_BaseAddress
     ```

5. **填入 Frida 脚本测试**

### 备选：使用 Signature 扫描
```javascript
// 在脚本中添加（使用前需要知道一些二进制特征）
const sig = findSignature(UNITY_BASE, '\x48\x8D\x0D\x00\x00\x00\x00\x48\x8B\x11', 0x1000000);
if (sig) {
    const GOM_OFFSET_FOUND = sig.sub(UNITY_BASE).toInt32();
    console.log('[GOM] 找到 GOM_OFFSET: ' + GOM_OFFSET_FOUND.toString(16));
}
```

---

## 📋 dump.cs 查找速查表

| 名称 | 搜索关键词 | 示例位置 |
|------|----------|--------|
| GameObject 类 | `public sealed class GameObject` | 约第 147765 行 |
| tag 字段 | `public string tag { get; }` | GameObject 类内部 |
| transform 字段 | `public Transform transform { get; }` | GameObject 类内部 |
| Transform 类 | `public class Transform : Component` | 需搜索 |
| position 字段 | `public Vector3 position { get; set; }` | Transform 类内部 |

---

## ✅ 快速验证清单

在填写 Frida 脚本前，确认以下事项：

- [ ] 在 dump.cs 中确认了 TAG_OFFSET（GameObject.tag）
- [ ] 在 dump.cs 中确认了 TRANSFORM_OFFSET（GameObject.transform）
- [ ] 在 dump.cs 中确认了 POS_OFFSET（Transform.position）
- [ ] 通过 CheatEngine 或其他方式找到了 GOM_OFFSET
- [ ] 更新 `GOM_Frida_Script.js` 中的四个常数
- [ ] （可选）找到了 UPDATE_OFFSET（任意 Update 函数的地址）
- [ ] 运行脚本后，输出的坐标看起来合理（不是全是 NaN）

---

## 🛠️ 常见问题

### Q：为什么 dump.cs 中没有 GameObjectManager？
**A：** 某些版本的 Unity IL2CPP 不导出这个内部管理器。此时可：
   1. 使用本项目已有的 `FindObjectsOfType()` API（推荐）
   2. 通过 CheatEngine 动态查找 GOM 地址

### Q：偏移值看起来是负数或非常大，对吗？
**A：** 不对。在 dump.cs 中的注释显示的偏移（如 `// 0x28`）始终是正的字节偏移。如果出现负数，通常是转换错误。

### Q：如何验证找到的偏移是否正确？
**A：** 
   1. 注入 Frida 脚本
   2. 观察输出的坐标：
      - ✅ 坐标在 [-1000, 1000] 范围内 → 正确
      - ❌ 全是 0 或 NaN → 偏移错误
      - ❌ 极端大数值（> 10^6）→ 偏移错误

### Q：我是否需要找到血量字段偏移？
**A：** 不一定。Frida 脚本中的血量是可选的（见代码注释）。如果你只需要位置信息，可以忽略 `PLAYER_HEALTH_OFFSET` 和 `ENEMY_HEALTH_OFFSET`。

---

## 📚 相关文档

- [遍历游戏对象坐标与绘制原理.md](../docs/遍历游戏对象坐标与绘制原理.md) - 项目的完整实现细节
- [IL2CPP_逆向分析笔记.md](../docs/IL2CPP_逆向分析笔记.md) - IL2CPP 基础知识
- `src/Engine/il2cpp_function.h` - 项目中已定义的所有 GameObject 操作函数

---

## 最后提醒

⚠️ **本脚本仅用于学习和研究目的。** 在真实游戏中使用前，确保符合当地法律和游戏协议。

