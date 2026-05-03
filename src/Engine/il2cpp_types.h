// ============================================================================
// 文件：src/Engine/il2cpp_types.h
// ============================================================================
//
// 【整个架构的核心概念 —— 什么是 IL2CPP？】
//
//   Unity 游戏用 C# 写逻辑，但 C# 不能直接运行在 CPU 上，需要编译成机器码。
//   IL2CPP 就是 Unity 的编译后端：把 C# 代码翻译成 C++ 代码，再编译成 GameAssembly.dll。
//   翻译后的 C++ 代码里，每个 C# 对象、数组、字符串都有固定的内存布局。
//   我们写外挂/DLL 注入时，需要知道这些布局才能正确读写游戏内存。
//
// 【这个文件的定位】
//
//   用 Il2CppDumper 工具对游戏的 GameAssembly.dll 进行逆向分析后，
//   会生成一个 sdk/il2cpp.h 文件（通常 30 万+ 行），里面包含了游戏所有类型定义。
//   但这个文件太大了，编译时会很慢，而且它只是参考用的。
//
//   所以本文件从 sdk/il2cpp.h 中只提取「项目实际需要用到的」几个核心结构体，
//   这样编译快，又能访问结构体成员。
//
// 【迁移到其他游戏时怎么办？】
//
//   1. 用 Il2CppDumper 导出新游戏的 dump.cs 和 il2cpp.h
//   2. 把 il2cpp.h 放到 sdk/ 目录下供参考
//   3. 从 il2cpp.h 中找到你需要的结构体，复制到本文件中
//   4. 常见需要提取的结构体：
//      - Il2CppObject（所有 C# 对象的头，必须有）
//      - Il2CppArrayBounds + il2cpp_array_size_t（数组遍历用）
//      - System_Object_array 或其他 Xxx_array（具体类型的数组布局）
//      - 你要 Hook 的游戏类（如 Player_array、Weapon_array 等）
//
// ============================================================================

#pragma once

#include <cstdint>
#include <cstddef>

// ----------------------------------------------------------------------------
// Il2CppObject —— 所有 C# 托管对象的"根"
// ----------------------------------------------------------------------------
//
// 【为什么需要这个结构体？】
//
//   在 IL2CPP 中，每一个 C# 对象（无论是 GameObject、Transform、还是 int 的装箱版本）
//   在内存中的前 16 字节永远是这两个字段：
//     +0x00  klass    → 指向该对象的类元信息（Il2CppClass*），相当于 C# 的 typeof()
//     +0x08  monitor  → 同步块/锁，用于 lock(obj) 等线程同步
//
//   知道这个布局后，我们就能：
//     - 从任意 C# 对象指针读取它的类型：obj->klass
//     - 用 il2cpp_object_get_class(obj) 也能达到同样效果
//
//   【内存示意】
//   偏移    字段
//   0x00    Il2CppClass* klass     ← "我是谁"
//   0x08    void*        monitor   ← "锁状态"
//   0x10    ... 后续字段由具体子类决定 ...
//
// ----------------------------------------------------------------------------
struct Il2CppObject
{
    Il2CppClass* klass;   // 对象所属的类元信息
    void*        monitor; // 同步块 / 监视器锁
};

// ----------------------------------------------------------------------------
// 数组相关类型 —— 遍历 C# 数组的基础
// ----------------------------------------------------------------------------
//
// 【IL2CPP 数组的内存布局】
//
//   C# 中的 T[] 数组在 IL2CPP 中的内存布局是：
//   +0x00  Il2CppObject  obj          ← 继承自对象头（klass + monitor）
//   +0x10  Il2CppArrayBounds* bounds  ← 多维数组时非空，一维数组通常为 nullptr
//   +0x18  il2cpp_array_size_t max_length  ← 数组元素个数
//   +0x20  T m_Items[...]            ← 实际数据，连续排列
//
//   所以要遍历一个 C# 数组，我们需要知道：
//     1. max_length —— 有多少个元素
//     2. m_Items    —— 元素从哪里开始
//
typedef uintptr_t il2cpp_array_size_t;         // 数组长度类型（64位下是 uint64_t）
typedef int32_t   il2cpp_array_lower_bound_t;  // 数组下界类型（C# 中通常为 0）

struct Il2CppArrayBounds
{
    il2cpp_array_size_t         length;      // 该维度的长度
    il2cpp_array_lower_bound_t lower_bound;  // 该维度的下界（C# 中通常为 0）
};

// ----------------------------------------------------------------------------
// System_Object_array —— Object[] 的真实内存布局
// ----------------------------------------------------------------------------
//
// 【为什么要单独定义这个结构体？】
//
//   当你调用 FindObjectsOfType 等方法时，返回值是一个 C# 的 Object[] 数组。
//   我们需要把这个 void* 指针转换成有具体字段的结构体，才能遍历数组元素。
//
//   【使用方法】
//   System_Object_array* arr = reinterpret_cast<System_Object_array*>(FindObjectsOfType(type));
//   for (size_t i = 0; i < arr->max_length; i++) {
//       Il2CppObject* obj = arr->m_Items[i];  // 拿到第 i 个对象
//   }
//
//   【关于 m_Items[1] —— 柔性数组】
//   C 语言中 [1] 是柔性数组技巧：结构体只声明 1 个元素的占位，
//   实际访问时可以越界到 max_length 个元素，因为数组元素在内存中是连续的。
//
//   【迁移到其他游戏】
//   如果你需要遍历其他类型的数组（如 Player[]），只需从 sdk/il2cpp.h 中
//   复制对应的结构体定义到本文件，如：
//     struct Player_array { Il2CppObject obj; ... Player* m_Items[1]; };
//
struct System_Object_array
{
    Il2CppObject        obj;         // 对象头（klass + monitor），所有 IL2CPP 对象都有
    Il2CppArrayBounds*  bounds;      // 多维数组时非空，一维数组通常为 nullptr
    il2cpp_array_size_t max_length;  // 元素总数 —— 遍历时用这个判断循环次数
    Il2CppObject*       m_Items[1];  // 柔性数组，实际长度 = max_length
};
