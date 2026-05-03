# Unity IL2CPP 对象结构分析笔记

## 一、核心概念：C# 托管对象 vs C++ 原生对象

Unity 的核心类型（GameObject、Transform、Component 等）采用**双对象架构**：

```
C# 托管对象 (Il2CppObject)          C++ 原生对象 (Native Object)
┌──────────────────────┐           ┌──────────────────────────┐
│ +0x00 Il2CppClass*   │           │ +0x00 m_Name             │
│ +0x08 monitor        │           │ +0x08 m_Tag              │
│ +0x10 m_CachedPtr ──────────────→│ +0x10 m_Layer            │
│       (8字节指针)     │           │ +0x18 m_IsActive         │
│                      │           │ +0x20 m_Transform        │
│ 实例大小: 24 字节     │           │ +0x28 m_Components[]     │
└──────────────────────┘           └──────────────────────────┘
  IL2CPP 可见，字段少                IL2CPP 不可见，字段多
  DumpObject 能遍历                  只能通过 CE 逆向或方法调用访问
```

### 为什么 DumpObject 只看到 m_CachedPtr？

因为 `GameObject`、`Transform`、`Component` 等 Unity 核心类的真实数据
全部存在 **C++ 原生侧**，C# 托管侧只是一个 24 字节的"薄包装器"，
唯一的实例字段就是 `m_CachedPtr`（指向 C++ 原生对象的指针）。

---

## 二、GameObject 实际内存布局（实测）

### C# 托管侧（Il2CppObject）

```
偏移    类型              字段名          说明
──────────────────────────────────────────────────────
0x00    Il2CppClass*      klass          类型元信息（所有对象共有）
0x08    void*             monitor        同步锁（所有对象共有）
0x10    void*             m_CachedPtr    ★ 指向 C++ 原生对象的指针
                                          这是访问真实数据的唯一入口

实例大小 = 24 字节（0x18）
```

### C++ 原生侧（需通过 CE 逆向确认）

```
偏移    类型              字段名          说明（推测，需 CE 验证）
──────────────────────────────────────────────────────
0x00    ???               m_Name         对象名称
0x08    ???               m_Tag          标签
0x10    int32_t           m_Layer        层级
0x18    bool              m_IsActive     是否激活
0x20    Transform*        m_Transform    Transform 组件
0x28    Component[]       m_Components   组件列表
...
```

> 注：C++ 原生侧偏移因 Unity 版本而异，必须用 CE 逐字节确认。

---

## 三、DumpClass 实测结果：GameObject 方法列表

### 属性方法（get/set）

| 属性 | Getter 地址 | Setter 地址 | 返回类型 |
|:---|:---|:---|:---|
| `transform` | `0x7FFDCC212E90` | — | `UnityEngine.Transform` |
| `layer` | `0x7FFDCC212E10` | `0x7FFDCC212ED0` | `System.Int32` |
| `activeSelf` | `0x7FFDCC212D90` | — | `System.Boolean` |
| `activeInHierarchy` | `0x7FFDCC212D50` | — | `System.Boolean` |
| `isStatic` | `0x7FFDCC212DD0` | — | `System.Boolean` |
| `tag` | `0x7FFDCC212E50` | — | `System.String` |
| `gameObject` | `0x7FFDCBBE3940` | — | `UnityEngine.GameObject` |

### 常用实例方法

| 方法 | 参数个数 | 返回类型 | 地址 |
|:---|:---|:---|:---|
| `GetComponent` | 1 | `Component` | `0x7FFDCC212840` |
| `GetComponentFastPath` | 2 | `void` | `0x7FFDCC212730` |
| `GetComponentInChildren` | 2 | `Component` | `0x7FFDCC212790` |
| `GetComponentInParent` | 1 | `Component` | `0x7FFDCC2127F0` |
| `GetComponentsInternal` | 6 | `Array` | `0x7FFDCC212950` |
| `SetActive` | 1 | `void` | `0x7FFDCC212A90` |
| `CompareTag` | 1 | `bool` | `0x7FFDCC212660` |
| `AddComponent` | 1 | `Component` | `0x7FFDCC212610` |
| `SendMessage` | 3 | `void` | `0x7FFDCC212A20` |
| `.ctor` | 0 | `void` | `0x7FFDCC212C40` |
| `.ctor` | 1 | `void` | `0x7FFDCC212CC0` |
| `.ctor` | 2 | `void` | `0x7FFDCC212B40` |

### 常用静态方法

| 方法 | 参数个数 | 返回类型 | 地址 |
|:---|:---|:---|:---|
| `Find` | 1 | `GameObject` | `0x7FFDCC2126F0` |
| `CreatePrimitive` | 1 | `GameObject` | `0x7FFDCC2126B0` |
| `Internal_CreateGameObject` | 2 | `void` | `0x7FFDCC2129D0` |
| `Internal_AddComponentWithType` | 1 | `Component` | `0x7FFDCC212610` |

### 泛型方法（地址为 0x0，需运行时实例化）

| 方法 | 说明 |
|:---|:---|
| `GetComponent<T>()` | 泛型版本，编译时无法确定地址 |
| `GetComponentInChildren<T>()` | 同上 |
| `TryGetComponent<T>()` | 同上 |
| `AddComponent<T>()` | 同上 |

---

## 四、DumpObject 实测结果：字段详解

### UnityEngine.Object（父类）

```
偏移      字段名                            类型              说明
──────────────────────────────────────────────────────────────────────
+0x0010   m_CachedPtr                       System.IntPtr     ★ 核心！指向 C++ 原生对象
+0x0000   OffsetOfInstanceIDInCPlusPlusObject System.Int32    静态常量，偏移0不代表实例字段
+0x0000   objectIsNullMessage               System.String     静态常量
+0x0000   cloneDestroyedMessage             System.String     静态常量
```

### 判断字段是否为实例字段

- **偏移 > 0x10**（跳过 klass + monitor）→ 实例字段
- **偏移 = 0x0000** → 静态字段（static），存储在 `il2cpp_class_get_static_field_data()` 返回的区域

---

## 五、访问 GameObject 数据的三种方式

### 方式1：通过 C# 方法调用（推荐，最稳定）

```cpp
// 1. 绑定方法指针（在 il2cpp_function.h 中添加）
DO_FUNC(Il2CppObject*, GameObject_get_transform, (void* thisPtr),
        "UnityEngine.CoreModule", "UnityEngine", "GameObject")
// 等价于属性: transform.get

// 2. 调用
Il2CppObject* transform = GameObject_get_transform(gameObjectPtr);
```

### 方式2：通过 m_CachedPtr 读取 C++ 原生对象

```cpp
// 1. 获取 m_CachedPtr（偏移 +0x10）
uintptr_t nativePtr = *(uintptr_t*)((uintptr_t)gameObjectPtr + 0x10);

// 2. 按 C++ 原生布局读取（偏移需 CE 验证）
int layer = *(int*)(nativePtr + 0x10);         // m_Layer（偏移为猜测）
void* transform = *(void**)(nativePtr + 0x20);  // m_Transform（偏移为猜测）
```

> ⚠️ C++ 原生偏移因 Unity 版本而异，每次更新可能变化！

### 方式3：通过 il2cpp_runtime_invoke 反射调用

```cpp
// 不需要提前声明方法签名，但调用复杂
const MethodInfo* method = il2cpp_class_get_method_from_name(pClass, "get_transform", 0);
Il2CppObject* exc = nullptr;
Il2CppObject* transform = il2cpp_runtime_invoke(method, gameObjectPtr, nullptr, &exc);
```

---

## 六、总结

| 特征 | C# 托管侧 | C++ 原生侧 |
|:---|:---|:---|
| **可见性** | DumpObject 可遍历 | 只能 CE 逆向 |
| **字段数** | 极少（仅 m_CachedPtr） | 丰富（m_Name, m_Layer 等） |
| **访问方式** | 直接读偏移 | 需通过 m_CachedPtr 间接访问 |
| **稳定性** | 跨版本稳定 | 偏移随版本变化 |
| **推荐方式** | 方法调用 | 仅在需要性能时使用 |

### 规律：哪些类是"薄包装器"？

- ✅ 薄包装器（字段只有 m_CachedPtr）：GameObject、Transform、Component、Behaviour、MonoBehaviour 等 Unity 核心类
- ❌ 非薄包装器（字段完整可见）：自定义 C# 类、System.String、数组等纯托管对象

判断标准：`DumpClass` 显示实例大小为 24 字节（0x18）→ 大概率是薄包装器
