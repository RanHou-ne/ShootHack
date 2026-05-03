// ============================================================================
// 文件：src/Engine/class.h
// ============================================================================
//
// 【这个文件是什么？—— 游戏特定类型的手动定义】
//
//   Il2CppDumper 生成的 sdk/il2cpp.h 包含了游戏所有类型的完整定义，
//   但它太大了（30 万+ 行），不参与编译。
//
//   本文件用于手动定义那些你实际需要访问内部成员的游戏类型。
//   和 il2cpp_types.h 的区别是：
//     - il2cpp_types.h 定义的是 IL2CPP 底层通用结构（Il2CppObject、数组布局等）
//     - class.h 定义的是游戏特定的上层类型（Unity_Array 等）
//
// 【Unity_Array 是什么？】
//
//   这是 Unity 中 Object[] 数组的简化版内存布局。
//   当 il2cpp_function.h 中声明 FindObjectsOfType 返回 Unity_Array* 时，
//   编译器需要知道 Unity_Array 是什么类型，所以需要在这里定义。
//
//   【和 System_Object_array 的关系】
//
//   System_Object_array 是从 dump 中提取的「精确布局」，字段完全匹配真实内存。
//   Unity_Array 是手动定义的「简化布局」，用偏移硬编码来访问成员。
//
//   两者的内存结构实际上是一样的（0x18 字节头 + 0x08 Count + 元素数组），
//   只是写法不同：
//     - System_Object_array: 用命名字段（obj, bounds, max_length, m_Items）
//     - Unity_Array: 用 pad 填充 + Count + Objects 数组
//
//   推荐优先使用 System_Object_array（定义在 il2cpp_types.h 中），
//   因为它的字段名和 dump 一致，更容易理解。
//
// 【迁移到其他游戏时怎么办？】
//
//   1. 如果你只需要遍历数组，直接用 il2cpp_types.h 中的 System_Object_array
//   2. 如果你要访问特定游戏类（如 Player、Weapon）的字段，
//      在本文件中添加对应的类定义，字段偏移从 dump.cs 中获取
//   3. 字段偏移的确定方法：
//      - 在 dump.cs 中找到类的字段顺序
//      - 每个字段的偏移 = 前面所有字段大小之和（考虑内存对齐）
//      - 或者用 Il2CppDumper 生成的 script.json 查看精确偏移
//
// ============================================================================

#pragma once

#include "../Core/Base.h"
#include "../Core/Struct.h"


// ----------------------------------------------------------------------------
// Unity_Array —— Unity 中 Object[] 的简化内存布局
// ----------------------------------------------------------------------------
// 这个结构体是手动定义的简化版本，包含了数组元素个数和元素指针数组。
// 注意：这个结构体不是从 dump.cs 中提取的，而是根据 Unity 数组的通用内存布局手动定义的。
// 内存布局：
//   偏移     内容
//   0x00     pad_0000[0x18]  ← 对应 Il2CppObject 头 (klass + monitor) + bounds 指针
//   0x18     Count           ← 数组元素个数（等价于 System_Object_array::max_length）
//   0x20     Objects[]       ← 元素数组起始（等价于 System_Object_array::m_Items）
//
class Unity_Array {
public:
    char pad_0000[0x18];     // 头部填充：klass(8) + monitor(8) + bounds(8) = 0x18 字节
    size_t Count;            // 0x20: 数组元素个数
    void* Objects[65536];    // 0x28: 元素指针数组（柔性数组近似，实际长度 = Count）
};


// ----------------------------------------------------------------------------
// Il2CppClass —— Unity 中的类信息结构
// ----------------------------------------------------------------------------
// 这个结构体是从 dump.cs 中提取的简化版本，包含了类名和命名空间等基本信息。
// 内存布局：
//   偏移     内容
//   0x00     pad_0000[0x10]  ← 前 16 字节未知（可能是 Il2CppObject 头或其他信息）
//   0x10     ClassName       ← 类名字符串指针
//   0x18     Namespace       ← 命名空间字符串指针
//  其他字段（如父类、接口等）未定义，因为目前不需要访问它们。

class Il2CppClass{
public:
    char pad_0000[0x10]; // 头部填充：前 16 字节未知（可能是 Il2CppObject 头或其他信息）
    const char* ClassName; // 0x10: 类名字符串指针 ，一个指针占 8 字节
    const char* Namespace;// 0x18: 命名空间字符串指针
};


// ----------------------------------------------------------------------------
// 本文件中 class 与 struct 的选用规则
// ----------------------------------------------------------------------------
//
//   class  —— 引用类型，像"快递箱"，箱子上贴着标签才找得到
//
//     内存布局（以 Unity_Array 为例）：
//       偏移 0x00  klass 指针    ← "箱子标签"：告诉 IL2CPP 这个对象是什么类型
//       偏移 0x08  monitor      ← "箱子锁"：  线程同步用，我们一般不管
//       偏移 0x10  bounds       ← 数组边界信息
//       偏移 0x18  Count        ← 真正有用的数据从这里开始
//       偏移 0x20  Objects[]    ← 元素数组
//
//     前面那 0x18 字节（klass + monitor + bounds）就是"头部"，
//     它是 IL2CPP 自动加上的管理信息，不是你自己定义的字段。
//     你要访问自己的字段，得跳过头部，所以 class 的字段前面都有 pad 填充。
//     使用方式：通过指针访问（Camera*、Transform*），不会整体拷贝。
//
//   struct —— 值类型，像"纸条"，直接写数据，没有额外包装
//
//     内存布局（以 Unity_Vector3 为例）：
//       偏移 0x00  x  ← 直接就是数据，没有任何多余的东西
//       偏移 0x04  y
//       偏移 0x08  z
//
//     没有 klass、没有 monitor、没有 pad，全是你自己定义的字段。
//     使用方式：函数返回时整块拷贝（函数签名写 Vector3 而不是 Vector3*）。
//
//   一句话总结：class 的字段前有 IL2CPP 自动加的管理信息（头部），struct 没有
//
// ----------------------------------------------------------------------------
// Unity 基础值类型（必须在 Camera/Transform/GameObject 等类之前定义）
// ----------------------------------------------------------------------------

struct Unity_Vector3
{
    float x;  // 世界坐标 X / 缩放 X
    float y;  // 世界坐标 Y / 缩放 Y
    float z;  // 世界坐标 Z / 缩放 Z
};

struct Unity_Quaternion
{
    float x;  // 旋转 X 分量
    float y;  // 旋转 Y 分量
    float z;  // 旋转 Z 分量
    float w;  // 旋转 W 分量（标量部分）
};

struct Unity_Vector2
{
    float x;
    float y;
};


// ----------------------------------------------------------------------------
// Camera —— Unity 摄像机
// ----------------------------------------------------------------------------
/*public sealed class Camera : Behaviour // TypeDefIndex: 2620
{
	// Fields
	public static Camera.CameraCallback onPreCull; // 0x0
	public static Camera.CameraCallback onPreRender; // 0x8
	public static Camera.CameraCallback onPostRender; // 0x10
*/

class Camera{
public:
    bool WorldToScreen(Vector3 position, Vector2& Point);
};

// ----------------------------------------------------------------------------
// Transform —— Unity 中的变换组件
// ----------------------------------------------------------------------------
// 简化版本，仅声明方法。Transform 实例通过 IL2CPP 函数指针操作，
// 不直接访问 C++ 成员字段。
class Transform{
public:
    Unity_Vector3 GetPosition();
};

// ----------------------------------------------------------------------------
// GameObject —— Unity 中的游戏对象
// ----------------------------------------------------------------------------
// 简化版本，仅声明方法。GameObject 实例通过 IL2CPP 函数指针操作。
class GameObject {
public:
    Transform* GetTransform();
};


class Unity_List{
    public:
    // IL2CPP List<T> 内存布局：
    // 0x00: Il2CppObject 头 (klass + monitor) = 16字节
    // 0x10: T[] _items — 内部数组指针（指向 Unity_Array）
    // 0x18: int _size — 实际元素个数（直接值，不是指针！）
    // 0x1C: int _version
    char pad_0000[0x10];
    Unity_Array* Objects;  // 0x10: _items，指向底层 T[] 数组
    int Count;             // 0x18: _size，实际元素个数
    int Version;           // 0x1C: _version
};



//
class GameManager_PVE{
    public:
    // Fields
    // 0x0000~0x022F: 头部 + EnimyPrefab(0x228, 8字节指针) 等不需要的字段
    char pad_0000[0x230];
    char pad_1000[0x8]; // 0x230
    char pad_2000[0x8]; // 0x238
    char pad_2001[0x8]; // 0x240
    int Money; // 0x248  
    Unity_List* StoreBuy; // 0x250
    int EnimyHard; // 0x258
    bool Firstswap; // 0x25C  
    int CurrentExistInScence; // 0x260  //当前这波每打死这几个人后一次性允许生成多少敌人(但是波仍然存在),这个跟WaveEnimy是相互作用,waveEnimy是这一波的总人数
    int CurrentWaveEnimy; // 0x264 //下波敌人数量
    int WaveEnimy; // 0x268  //这一波中还剩多少敌人未生成
    int WaveCount; // 0x26C  //波数
    bool CanInsEnimy; // 0x270 //改为0会变成负数倒计时,相当于变成计时器了,然后敌人也不会生产了
    float WaveTime; // 0x274  //默认值是初始值,如果大于初始值会增加下一波的时间,相反小于则减少,但只体现在ui上,实际没变化还是20s
    char pad_3000[0x8]; // 0x278
    char pad_4000[0x8]; // 0x280
    char pad_5000[0x8]; // 0x288
    bool IsAddUI; // 0x290
    char pad_291[0x7]; // 0x291 对齐到8字节
    Unity_List* PVE_EnimyList; // 0x298
};