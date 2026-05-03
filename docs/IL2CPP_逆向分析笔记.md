# IL2CPP 函数指针获取与对象结构分析笔记

## 一、获取函数指针的完整流程

### 1. 整体架构图

```
DLL注入 → DllMain → Engine::Initialize()
                           │
                           ├─ 1. FindWindowA("UnityWndClass") → 获取窗口句柄
                           │
                           ├─ 2. GetModuleHandleA → 获取 DLL 句柄
                           │     ├─ UnityPlayer.dll  → 渲染相关
                           │     └─ GameAssembly.dll → IL2CPP 核心
                           │
                           ├─ 3. GetProcAddress 绑定 il2cpp_xxx 官方 API
                           │
                           └─ 4. GetMethod 绑定游戏 C# 方法
```

### 2. IL2CPP 官方 C API（GetProcAddress）

通过 X-Macro 机制，对 `il2cpp_api.h` 中声明的每个 API：

```cpp
DO_API(Il2CppDomain*, il2cpp_domain_get, ())
```

展开为：

```cpp
il2cpp_domain_get = reinterpret_cast<il2cpp_domain_get_t>(
    GetProcAddress(g_engineGameAssembly, "il2cpp_domain_get"));
```

**原理**：IL2CPP 的导出函数名是固定的（`il2cpp_xxx`），`GetProcAddress` 直接在 PE 导出表中按名称查找返回地址。

### 3. 游戏 C# 方法（GetMethod）

通过 X-Macro 机制，对 `il2cpp_function.h` 中声明的每个方法：

```cpp
DO_FUNC(Unity_Array*, FindObjectsOfType, (void*),
        "UnityEngine.CoreModule", "UnityEngine", "Object")
```

展开为：

```cpp
FindObjectsOfType = reinterpret_cast<FindObjectsOfType_t>(
    GetMethod("UnityEngine.CoreModule", "UnityEngine", "Object", "FindObjectsOfType"));
```

### 4. GetMethod 内部实现（IL2CPP 层次结构）

```
Domain → Assembly → Image → Class → MethodInfo → *(void**) → 函数指针
  ↑          ↑         ↑        ↑         ↑
il2cpp_   il2cpp_   il2cpp_  il2cpp_   il2cpp_class_
domain_get domain_  assembly class_    get_method_
           assembly_ get_image from_   from_name
           open               name
```

**为什么 C# 方法不能用 GetProcAddress？**
因为 IL2CPP 编译后的方法名是混淆过的且不导出，只能通过元数据遍历定位。

### 5. 两类函数指针对比

| | IL2CPP 官方 API | 游戏 C# 方法 |
|:---|:---|:---|
| **获取方式** | `GetProcAddress` | `GetMethod`（元数据遍历） |
| **查找依据** | 固定导出名 `il2cpp_xxx` | 程序集+命名空间+类名+方法名 |
| **宏** | `DO_API` | `DO_FUNC` |
| **声明文件** | `il2cpp_api.h` | `il2cpp_function.h` |

---

## 二、Unity/IL2CPP 对象内存布局

### 通用 GameObject 内存布局（IL2CPP）

```
偏移      类型           说明
─────────────────────────────────────
+0x00     Pointer        Il2CppClass* / vtable（对象的类型信息）
+0x08     4 Bytes        syncBlockIndex（同步块或引用计数）
+0x10     Pointer        m_Name（对象名称字符串指针）
+0x18     Pointer        m_Tag（标签字符串指针）
+0x20     4 Bytes        m_InstanceID（Unity 实例 ID）
+0x28     4 Bytes        m_Layer 或标志位
+0x30     4 Bytes        可能是另一个 ID 或计数
+0x34     Double         可能是时间戳或某种浮点属性
+0x40     Pointer        m_Transform（Transform 组件指针）
+0x48     Pointer        可能是另一个组件或重复引用
+0x58     Pointer        m_Components（组件列表数组）
+0x68     Pointer        可能是父对象或场景引用
```

> 注：以上为 Unity 引擎的通用布局经验值，具体游戏版本可能有差异。

### TestFindObjectsOfType 返回值结构

```cpp
System_Object_array* ObjArray = FindObjectsOfType(pTypeObj);
// ObjArray->max_length  → 数组长度
// ObjArray->m_Items[i]  → System_Object* 数组元素
// m_Items[i]->klass    → Il2CppClass*（可获取真实类型名）
```

---

## 三、准确分析结构的方法

### 方法一：il2cppDumper（推荐）

**工具**：https://github.com/Perfare/Il2CppDumper

运行后生成：
- `script.py` — IDA/GHIDRA 导入脚本
- `dump.cs` — C# 类型定义文件
- `strings.txt` — 所有字符串常量

**生成的文件中包含完整的类结构定义**，可以直接看到每个字段的偏移、类型、名称。

### 方法二：Il2CppInspector

**工具**：https://github.com/MaxMynter/Il2CppInspector

支持生成：
- IDA Python 脚本
- .h 头文件（可直接给游戏 DLL 用）
- JSON 结构定义

### 方法三：Unity 开源代码

Unity 在 GitHub 有开源代码：

```
https://github.com/Unity-Technologies/UnityCsReference
```

搜索 `Object.hpp`、`GameObject.hpp` 等文件，对照分析。

### 方法四：动态遍历字段

利用 IL2CPP API 动态打印对象结构：

```cpp
#include <cstdio>

void DumpObjectInfo(Il2CppObject* pObj)
{
    if (!pObj) return;

    // 获取对象的真实类型名
    Il2CppClass* pClass = il2cpp_object_get_class(pObj);
    const char* className = il2cpp_class_get_name(pClass);
    const char* nameSpace = il2cpp_class_get_namespace(pClass);
    printf("[*] Type: %s.%s\n", nameSpace, className);

    // 遍历所有字段
    void* iter = nullptr;
    while (Il2CppField* field = il2cpp_class_get_fields(pClass, &iter))
    {
        const char* fieldName = il2cpp_field_get_name(field);
        Il2CppType* fieldType = il2cpp_field_get_type(field);
        int offset = il2cpp_field_get_offset(field);
        printf("  +0x%02X %s (%s)\n",
               offset,
               fieldName,
               il2cpp_type_get_name(fieldType));
    }
}
```

---

## 四、推荐分析流程

| 步骤 | 工具/方法 | 输出 |
|:---|:---|:---|
| 1 | il2cppDumper | 完整类型定义 + 偏移 |
| 2 | 生成的 dump.cs | C# 结构体代码 |
| 3 | 生成的 script.py | IDA/GHIDRA 数据库 |
| 4 | 动态遍历字段 | 实时验证偏移 |

**最快的方式是跑一遍 il2cppDumper**，它会自动生成目标游戏版本的准确结构文件。

---

## 五、相关文件位置

本项目相关文件：
- `src/Engine/Engine.cpp` — 引擎初始化与 GetMethod 实现
- `src/Engine/il2cpp_api.h` — IL2CPP 官方 API 声明
- `src/Engine/il2cpp_function.h` — 游戏 C# 方法声明
- `sdk/` — 可能有 Unity 版本类型定义
