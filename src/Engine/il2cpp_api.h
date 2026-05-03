// ============================================================================
// 文件：src/Engine/il2cpp_api.h
// ============================================================================
//
// 【这个文件是什么？—— IL2CPP 官方 C API 函数列表】
//
//   Unity 的 GameAssembly.dll 会导出一系列以 il2cpp_ 开头的 C 函数，
//   这些是 IL2CPP 运行时对外暴露的「官方接口」，可以用来：
//     - 遍历所有程序集、类、方法、字段
//     - 创建对象、调用方法、读写字段
//     - 获取类型信息、方法签名等元数据
//
//   本文件用 DO_API 宏把这些函数列成一张表，和 il2cpp_function.h 一样是 X-Macro 数据表。
//
// 【DO_API 和 DO_FUNC 的区别】
//
//   ┌────────────┬────────────────────────────────────────────────────────┐
//   │            │ DO_API                                    │ DO_FUNC              │
//   ├────────────┼────────────────────────────────────────────────────────┤
//   │ 对应什么    │ GameAssembly.dll 导出的 C 函数（il2cpp_xxx）              │ 游戏中的 C# 方法         │
//   │ 参数个数    │ 3 个 (返回类型, 函数名, 参数列表)                          │ 6 个 (多了程序集/命名空间/类名) │
//   │ 怎么获取地址 │ GetProcAddress(hModule, "il2cpp_xxx")                  │ GetMethod(程序集,命名空间,类名,方法名) │
//   │ 函数名规则  │ 固定前缀 il2cpp_，全局唯一                               │ 可能有重名，需要完整路径定位   │
//   │ 通用性      │ 所有 IL2CPP 游戏都一样（跨游戏通用）                       │ 每个游戏不同，需要看 dump.cs │
//   └────────────┴────────────────────────────────────────────────────────┘
//
// 【这些 API 从哪来的？】
//
//   它们是 Unity 引擎固定的导出函数，不管什么游戏，只要用 IL2CPP 编译，
//   GameAssembly.dll 都会导出这些函数。你不需要用 Il2CppDumper 生成它们。
//   完整列表可以参考 Unity 源码：il2cpp/il2cpp-api.h
//
// 【迁移到其他游戏时怎么办？】
//
//   这个文件基本不需要改！因为 IL2CPP C API 在所有游戏中都一样。
//   你只需要：
//     1. 确认游戏确实用的是 IL2CPP（而不是 Mono）
//     2. 如果新版本 Unity 增加了新 API，可以在本文件中添加
//     3. 如果某个 API 在旧版本中不存在，GetProcAddress 会返回 nullptr，
//        使用前判空即可
//
// 【X-Macro 展开流程图】
//
//   本文件被 #include 三次，每次 DO_API 的含义不同：
//
//   ┌─────────────────┐    #define DO_API → using X_t = ...; extern X_t X;
//   │   Engine.h      │───→ #include "il2cpp_api.h"  ← 声明阶段
//   │                 │    #undef DO_API
//   └─────────────────┘
//
//   ┌─────────────────┐    #define DO_API → X_t X = nullptr;
//   │  Engine.cpp     │───→ #include "il2cpp_api.h"  ← 定义阶段
//   │  (全局作用域)    │    #undef DO_API
//   └─────────────────┘
//
//   ┌─────────────────┐    #define DO_API → X = (X_t)GetProcAddress(...);
//   │  Engine.cpp     │───→ #include "il2cpp_api.h"  ← 绑定阶段（在 Initialize 内）
//   │  (Initialize内) │    #undef DO_API
//   └─────────────────┘
//
// ============================================================================

// ============================================================================
// 前向声明 —— 仅供 IntelliSense（IDE 代码提示）使用
// ============================================================================
// 当 IDE 单独解析本文件时，DO_API 宏未定义，这些 typedef 确保类型名已知。
// 在实际编译时，Engine.h 已在 #include 本文件之前完成了所有前向声明和完整定义，
// 这些 typedef 会被跳过，避免与 Engine.h / class.h 中的定义产生 ODR 冲突。
#ifndef DO_API
typedef class Il2CppClass Il2CppClass;
typedef struct Il2CppType Il2CppType;
typedef struct EventInfo EventInfo;
typedef struct MethodInfo MethodInfo;
typedef struct FieldInfo FieldInfo;
typedef struct PropertyInfo PropertyInfo;

typedef struct Il2CppAssembly Il2CppAssembly;
typedef struct Il2CppArray Il2CppArray;
typedef struct Il2CppDelegate Il2CppDelegate;
typedef struct Il2CppDomain Il2CppDomain;
typedef struct Il2CppImage Il2CppImage;
typedef struct Il2CppException Il2CppException;
typedef struct Il2CppProfiler Il2CppProfiler;
typedef struct Il2CppObject Il2CppObject;
typedef struct Il2CppReflectionMethod Il2CppReflectionMethod;
typedef struct Il2CppReflectionType Il2CppReflectionType;
typedef struct Il2CppString Il2CppString;
typedef struct Il2CppThread Il2CppThread;
typedef struct Il2CppAsyncResult Il2CppAsyncResult;
typedef struct Il2CppManagedMemorySnapshot Il2CppManagedMemorySnapshot;
typedef struct Il2CppCustomAttrInfo Il2CppCustomAttrInfo;
#endif // DO_API

// Il2CppChar 不是前向声明，而是类型别名，无论 DO_API 是否定义都需要。
// 如果放在 #ifndef DO_API 内，实际编译时（DO_API 已定义）会导致 Il2CppChar 未定义。
#include <cstdint>
typedef uint16_t Il2CppChar;

// ============================================================================
// Assembly（程序集）相关 API
// ============================================================================
// 程序集 = 编译后的 .dll 文件，包含多个命名空间和类。
// IL2CPP 的层次结构：Domain → Assembly[] → Image → Class[] → Method[]/Field[]
//
//   Domain（域）
//   └── Assembly（程序集，如 "UnityEngine.CoreModule"）
//       └── Image（镜像，Assembly 的元数据视图）
//           └── Class（类，如 "UnityEngine.GameObject"）
//               ├── Method（方法，如 "SetActive"）
//               └── Field（字段，如 "m_Name"）
//
DO_API(const Il2CppImage*, il2cpp_assembly_get_image, (const Il2CppAssembly* assembly));
// 从程序集获取 Image（镜像），Image 是查找类和方法的入口
// 用法: const Il2CppImage* img = il2cpp_assembly_get_image(pAssembly);

// ============================================================================
// Class（类）相关 API
// ============================================================================
// Class = C# 中的类型定义，包含字段、方法、属性等成员。
// 这是最常用的一组 API，几乎所有操作都要先拿到 Il2CppClass*。
DO_API(void, il2cpp_class_for_each, (void(*klassReportFunc)(Il2CppClass* klass, void* userData), void* userData));
// 遍历所有已加载的类，对每个类调用回调函数（用于枚举/搜索）

DO_API(const Il2CppType*, il2cpp_class_enum_basetype, (Il2CppClass* klass));
// 获取枚举类型的基础类型（如 enum Color : int → 返回 int 的 Il2CppType*）

DO_API(bool, il2cpp_class_is_generic, (const Il2CppClass* klass));
// 判断类是否是泛型定义（如 List<T>，T 还未确定）

DO_API(bool, il2cpp_class_is_inflated, (const Il2CppClass* klass));
// 判断类是否是已实例化的泛型（如 List<int>，T 已确定为 int）

DO_API(bool, il2cpp_class_is_assignable_from, (Il2CppClass* klass, Il2CppClass* oklass));
// 判断 oklass 是否可以赋值给 klass（等价于 C# 的 is/AssignableFrom）

DO_API(bool, il2cpp_class_is_subclass_of, (Il2CppClass* klass, Il2CppClass* klassc, bool check_interfaces));
// 判断 klass 是否是 klassc 的子类

DO_API(bool, il2cpp_class_has_parent, (Il2CppClass* klass, Il2CppClass* klassc));
// 判断 klass 是否直接继承 klassc

DO_API(Il2CppClass*, il2cpp_class_from_il2cpp_type, (const Il2CppType* type));
// 从 Il2CppType* 获取对应的 Il2CppClass*

DO_API(Il2CppClass*, il2cpp_class_from_name, (const Il2CppImage* image, const char* namespaze, const char* name));
// ★ 最常用！根据命名空间和类名查找类
// 用法: Il2CppClass* cls = il2cpp_class_from_name(img, "UnityEngine", "GameObject");

DO_API(Il2CppClass*, il2cpp_class_from_system_type, (Il2CppReflectionType* type));
// 从 System.Type 反射对象获取 Il2CppClass*

DO_API(Il2CppClass*, il2cpp_class_get_element_class, (Il2CppClass* klass));
// 获取数组元素的类型（如 int[] → 返回 int 的 Il2CppClass*）

DO_API(const EventInfo*, il2cpp_class_get_events, (Il2CppClass* klass, void** iter));
// 遍历类中所有事件（用迭代器模式，iter 初始传 nullptr）

DO_API(FieldInfo*, il2cpp_class_get_fields, (Il2CppClass* klass, void** iter));
// 遍历类中所有字段（用迭代器模式）
// 用法: void* iter = nullptr; while (auto* f = il2cpp_class_get_fields(cls, &iter)) { ... }

DO_API(Il2CppClass*, il2cpp_class_get_nested_types, (Il2CppClass* klass, void** iter));
// 遍历嵌套类型

DO_API(Il2CppClass*, il2cpp_class_get_interfaces, (Il2CppClass* klass, void** iter));
// 遍历类实现的所有接口

DO_API(const PropertyInfo*, il2cpp_class_get_properties, (Il2CppClass* klass, void** iter));
// 遍历类中所有属性

DO_API(const PropertyInfo*, il2cpp_class_get_property_from_name, (Il2CppClass* klass, const char* name));
// 按名称查找属性

DO_API(FieldInfo*, il2cpp_class_get_field_from_name, (Il2CppClass* klass, const char* name));
// ★ 常用！按名称查找字段，返回 FieldInfo*（可用于 il2cpp_field_get_offset 计算偏移）

DO_API(const MethodInfo*, il2cpp_class_get_methods, (Il2CppClass* klass, void** iter));
// 遍历类中所有方法

DO_API(const MethodInfo*, il2cpp_class_get_method_from_name, (Il2CppClass* klass, const char* name, int argsCount));
// ★ 最常用！按名称查找方法，argsCount=-1 表示忽略参数个数
// 用法: const MethodInfo* m = il2cpp_class_get_method_from_name(cls, "Update", -1);

DO_API(const char*, il2cpp_class_get_name, (Il2CppClass* klass));
// 获取类名（如 "GameObject"）

DO_API(void, il2cpp_type_get_name_chunked, (const Il2CppType* type, void(*chunkReportFunc)(void* data, void* userData), void* userData));
// 分块获取类型名称（用于超长名称）

DO_API(const char*, il2cpp_class_get_namespace, (Il2CppClass* klass));
// 获取命名空间（如 "UnityEngine"）

DO_API(Il2CppClass*, il2cpp_class_get_parent, (Il2CppClass* klass));
// 获取父类

DO_API(Il2CppClass*, il2cpp_class_get_declaring_type, (Il2CppClass* klass));
// 获取声明类型（用于嵌套类）

DO_API(int32_t, il2cpp_class_instance_size, (Il2CppClass* klass));
// 获取类实例的大小（字节），用于内存分配

DO_API(size_t, il2cpp_class_num_fields, (const Il2CppClass* enumKlass));
// 获取字段数量

DO_API(bool, il2cpp_class_is_valuetype, (const Il2CppClass* klass));
// 判断是否是值类型（struct）

DO_API(int32_t, il2cpp_class_value_size, (Il2CppClass* klass, uint32_t* align));
// 获取值类型的大小

DO_API(bool, il2cpp_class_is_blittable, (const Il2CppClass* klass));
// 判断是否可以直接复制到非托管内存

DO_API(int, il2cpp_class_get_flags, (const Il2CppClass* klass));
// 获取类型标志（abstract、sealed 等）

DO_API(bool, il2cpp_class_is_abstract, (const Il2CppClass* klass));
// 判断是否是抽象类

DO_API(bool, il2cpp_class_is_interface, (const Il2CppClass* klass));
// 判断是否是接口

DO_API(int, il2cpp_class_array_element_size, (Il2CppClass* klass));
// 获取数组元素大小（用于计算数组中每个元素的跨度）

DO_API(Il2CppClass*, il2cpp_class_from_type, (const Il2CppType* type));
// 从 Il2CppType* 获取 Il2CppClass*（和 il2cpp_class_from_il2cpp_type 功能相同）

DO_API(const Il2CppType*, il2cpp_class_get_type, (Il2CppClass* klass));
// ★ 常用！从 Il2CppClass* 获取 Il2CppType*
// 用法: const Il2CppType* type = il2cpp_class_get_type(cls);

DO_API(uint32_t, il2cpp_class_get_type_token, (Il2CppClass* klass));
// 获取类型的 metadata token

DO_API(bool, il2cpp_class_has_attribute, (Il2CppClass* klass, Il2CppClass* attr_class));
// 判断类是否有指定特性（Attribute）

DO_API(bool, il2cpp_class_has_references, (Il2CppClass* klass));
// 判断类是否包含 GC 引用

DO_API(bool, il2cpp_class_is_enum, (const Il2CppClass* klass));
// 判断是否是枚举类型

DO_API(const Il2CppImage*, il2cpp_class_get_image, (Il2CppClass* klass));
// 获取类所在的 Image

DO_API(const char*, il2cpp_class_get_assemblyname, (const Il2CppClass* klass));
// 获取类所在的程序集名称

DO_API(int, il2cpp_class_get_rank, (const Il2CppClass* klass));
// 获取数组的维度（rank=1 表示一维数组）

DO_API(uint32_t, il2cpp_class_get_data_size, (const Il2CppClass* klass));
// 获取静态字段数据大小

DO_API(void*, il2cpp_class_get_static_field_data, (const Il2CppClass* klass));
// ★ 常用！获取静态字段的内存区域基址
// 读取静态字段值时需要：基址 + 字段偏移 = 字段值地址

// ============================================================================
// Domain（运行时域）相关 API
// ============================================================================
// Domain 是 IL2CPP 运行时的最顶层容器，相当于整个游戏环境。
// 一切查找操作都从获取 Domain 开始。
DO_API(Il2CppDomain*, il2cpp_domain_get, ());
// ★ 核心！获取 IL2CPP 运行时域，几乎所有操作的起点
// 用法: Il2CppDomain* domain = il2cpp_domain_get();

DO_API(const Il2CppAssembly*, il2cpp_domain_assembly_open, (Il2CppDomain* domain, const char* name));
// ★ 核心！打开指定名称的程序集
// 用法: const Il2CppAssembly* asm = il2cpp_domain_assembly_open(domain, "UnityEngine.CoreModule");

DO_API(const Il2CppAssembly**, il2cpp_domain_get_assemblies, (const Il2CppDomain* domain, size_t* size));
// 获取所有已加载的程序集列表

// ============================================================================
// Field（字段）相关 API
// ============================================================================
// 字段 = C# 类中的成员变量。通过这些 API 可以读取/修改对象的字段值。
DO_API(int, il2cpp_field_get_flags, (FieldInfo* field));
// 获取字段的访问标志（public/private/static 等）

DO_API(const char*, il2cpp_field_get_name, (FieldInfo* field));
// 获取字段名

DO_API(Il2CppClass*, il2cpp_field_get_parent, (FieldInfo* field));
// 获取字段所属的类

DO_API(size_t, il2cpp_field_get_offset, (FieldInfo* field));
// ★ 核心！获取字段在对象内存中的偏移量
// 用法: size_t offset = il2cpp_field_get_offset(field);
//       int* valuePtr = (int*)((uintptr_t)obj + offset);  // 读取字段值

DO_API(const Il2CppType*, il2cpp_field_get_type, (FieldInfo* field));
// 获取字段的类型信息

DO_API(void, il2cpp_field_get_value, (Il2CppObject* obj, FieldInfo* field, void* value));
// 读取对象的字段值（复制到 value 缓冲区）

DO_API(Il2CppObject*, il2cpp_field_get_value_object, (FieldInfo* field, Il2CppObject* obj));
// 读取对象的字段值（返回装箱后的 Il2CppObject*）

DO_API(bool, il2cpp_field_has_attribute, (FieldInfo* field, Il2CppClass* attr_class));
// 判断字段是否有指定特性

DO_API(void, il2cpp_field_set_value, (Il2CppObject* obj, FieldInfo* field, void* value));
// 设置对象的字段值

DO_API(void, il2cpp_field_static_get_value, (FieldInfo* field, void* value));
// ★ 常用！读取静态字段的值
// 用法: float staticValue; il2cpp_field_static_get_value(field, &staticValue);

DO_API(void, il2cpp_field_static_set_value, (FieldInfo* field, void* value));
// ★ 常用！设置静态字段的值（可用于修改全局状态）

DO_API(void, il2cpp_field_set_value_object, (Il2CppObject* instance, FieldInfo* field, Il2CppObject* value));
// 用 Il2CppObject* 设置字段值

DO_API(bool, il2cpp_field_is_literal, (FieldInfo* field));
// 判断字段是否是 const（编译时常量）

// ============================================================================
// Method（方法）相关 API
// ============================================================================
DO_API(const Il2CppType*, il2cpp_method_get_return_type, (const MethodInfo* method));
// 获取方法的返回类型

DO_API(Il2CppClass*, il2cpp_method_get_declaring_type, (const MethodInfo* method));
// 获取方法所属的类

DO_API(const char*, il2cpp_method_get_name, (const MethodInfo* method));
// 获取方法名

DO_API(const MethodInfo*, il2cpp_method_get_from_reflection, (const Il2CppReflectionMethod* method));
// 从反射方法对象获取 MethodInfo

DO_API(Il2CppReflectionMethod*, il2cpp_method_get_object, (const MethodInfo* method, Il2CppClass* refclass));
// 从 MethodInfo 创建反射方法对象

DO_API(bool, il2cpp_method_is_generic, (const MethodInfo* method));
// 判断方法是否是泛型

DO_API(bool, il2cpp_method_is_inflated, (const MethodInfo* method));
// 判断方法是否是已实例化的泛型

DO_API(bool, il2cpp_method_is_instance, (const MethodInfo* method));
// 判断方法是否是实例方法（非 static）

DO_API(uint32_t, il2cpp_method_get_param_count, (const MethodInfo* method));
// 获取方法参数个数

DO_API(const Il2CppType*, il2cpp_method_get_param, (const MethodInfo* method, uint32_t index));
// 获取指定位置的参数类型

DO_API(Il2CppClass*, il2cpp_method_get_class, (const MethodInfo* method));
// 获取方法所属的类

DO_API(bool, il2cpp_method_has_attribute, (const MethodInfo* method, Il2CppClass* attr_class));
// 判断方法是否有指定特性

DO_API(uint32_t, il2cpp_method_get_flags, (const MethodInfo* method, uint32_t* iflags));
// 获取方法标志

DO_API(uint32_t, il2cpp_method_get_token, (const MethodInfo* method));
// 获取方法的 metadata token

DO_API(const char*, il2cpp_method_get_param_name, (const MethodInfo* method, uint32_t index));
// 获取参数名

// ============================================================================
// Property（属性）相关 API
// ============================================================================
DO_API(uint32_t, il2cpp_property_get_flags, (PropertyInfo* prop));
// 获取属性标志

DO_API(const MethodInfo*, il2cpp_property_get_get_method, (PropertyInfo* prop));
// 获取属性的 getter 方法

DO_API(const MethodInfo*, il2cpp_property_get_set_method, (PropertyInfo* prop));
// 获取属性的 setter 方法

DO_API(const char*, il2cpp_property_get_name, (PropertyInfo* prop));
// 获取属性名

DO_API(Il2CppClass*, il2cpp_property_get_parent, (PropertyInfo* prop));
// 获取属性所属的类

// ============================================================================
// Thread（线程）相关 API
// ============================================================================
DO_API(Il2CppThread*, il2cpp_thread_current, ());
// 获取当前线程

DO_API(Il2CppThread*, il2cpp_thread_attach, (Il2CppDomain* domain));
// 将当前原生线程附加到 IL2CPP 域（从非 C# 线程调用 C# 方法时必须先 attach）
// ★ 重要！DLL 注入后，如果你的代码在非 Unity 线程中调用 C# 方法，必须先 attach

DO_API(void, il2cpp_thread_detach, (Il2CppThread* thread));
// 分离线程

DO_API(Il2CppThread**, il2cpp_thread_get_all_attached_threads, (size_t* size));
// 获取所有已附加的线程

DO_API(bool, il2cpp_is_vm_thread, (Il2CppThread* thread));
// 判断是否是 VM 线程

// ============================================================================
// Runtime（运行时调用）相关 API
// ============================================================================
DO_API(Il2CppObject*, il2cpp_runtime_invoke, (const MethodInfo* method, void* obj, void** params, Il2CppException** exc));
// ★ 核心！通过 MethodInfo 反射调用 C# 方法
// 参数: method  → 要调用的方法
//       obj     → this 指针（静态方法传 nullptr）
//       params  → 参数数组（每个元素是 void* 指向参数值）
//       exc     → 输出异常（可传 nullptr 忽略）
// 返回: 方法返回值的 Il2CppObject*（void 方法返回 nullptr）

// ============================================================================
// String（字符串）相关 API
// ============================================================================
DO_API(Il2CppString*, il2cpp_string_new, (const char* str));
// ★ 常用！从 C 字符串创建 C# 托管字符串
// 用法: Il2CppString* csStr = il2cpp_string_new("Hello");

DO_API(int32_t, il2cpp_string_length, (Il2CppString* str));
// 获取字符串长度

DO_API(const Il2CppChar*, il2cpp_string_chars, (Il2CppString* str));
// ★ 常用！获取字符串的字符数据指针（UTF-16 编码）
// 用法: const wchar_t* text = (const wchar_t*)il2cpp_string_chars(csStr);

// ============================================================================
// Type（类型元信息）相关 API
// ============================================================================
DO_API(Il2CppObject*, il2cpp_type_get_object, (const Il2CppType* type));
// ★ 核心！将 Il2CppType* 包装成 C# 的 System.Type 实例
// 很多 C# 方法（如 FindObjectsOfType）需要 System.Type 参数，必须用这个转换

DO_API(int, il2cpp_type_get_type, (const Il2CppType* type));
// 获取类型的枚举值（VALUETYPE, CLASS, GENERICINST 等）

DO_API(Il2CppClass*, il2cpp_type_get_class_or_element_class, (const Il2CppType* type));
// 从类型获取类（泛型时获取元素类型）

DO_API(char*, il2cpp_type_get_name, (const Il2CppType* type));
// ★ 常用！获取类型名称字符串（调试用）
// 用法: char* name = il2cpp_type_get_name(type);  // 如 "UnityEngine.GameObject"

DO_API(bool, il2cpp_type_is_byref, (const Il2CppType* type));
// 判断是否是引用参数（ref/out）

DO_API(uint32_t, il2cpp_type_get_attrs, (const Il2CppType* type));
// 获取类型属性

DO_API(bool, il2cpp_type_equals, (const Il2CppType* type, const Il2CppType* otherType));
// 判断两个类型是否相等

DO_API(char*, il2cpp_type_get_assembly_qualified_name, (const Il2CppType* type));
// 获取程序集限定名（如 "UnityEngine.GameObject, UnityEngine.CoreModule"）

DO_API(bool, il2cpp_type_is_static, (const Il2CppType* type));
// 判断是否是静态类型

DO_API(bool, il2cpp_type_is_pointer_type, (const Il2CppType* type));
// 判断是否是指针类型

// ============================================================================
// Image（模块镜像）相关 API
// ============================================================================
DO_API(const Il2CppAssembly*, il2cpp_image_get_assembly, (const Il2CppImage* image));
// 获取 Image 所属的 Assembly

DO_API(const char*, il2cpp_image_get_name, (const Il2CppImage* image));
// 获取 Image 名称（如 "UnityEngine.CoreModule"）

DO_API(const char*, il2cpp_image_get_filename, (const Il2CppImage* image));
// 获取 Image 对应的文件路径

DO_API(const MethodInfo*, il2cpp_image_get_entry_point, (const Il2CppImage* image));
// 获取程序集入口方法（Main 函数）

DO_API(size_t, il2cpp_image_get_class_count, (const Il2CppImage* image));
// 获取 Image 中的类数量

DO_API(const Il2CppClass*, il2cpp_image_get_class, (const Il2CppImage* image, size_t index));
// 按索引获取 Image 中的类

// ============================================================================
// Object（对象实例）相关 API
// ============================================================================
DO_API(Il2CppClass*, il2cpp_object_get_class, (Il2CppObject* obj));
// 获取对象的类型（Il2CppClass*）

DO_API(uint32_t, il2cpp_object_get_size, (Il2CppObject* obj));
// 获取对象实例大小

DO_API(const MethodInfo*, il2cpp_object_get_virtual_method, (Il2CppObject* obj, const MethodInfo* method));
// 获取对象的虚方法实现（用于正确调用虚方法/重写方法）

DO_API(Il2CppObject*, il2cpp_object_new, (const Il2CppClass* klass));
// ★ 常用！创建新的 C# 对象实例（相当于 new ClassName()）
// 用法: Il2CppObject* obj = il2cpp_object_new(pClass);

DO_API(void*, il2cpp_object_unbox, (Il2CppObject* obj));
// 对装箱的值类型拆箱，获取内部值数据指针
