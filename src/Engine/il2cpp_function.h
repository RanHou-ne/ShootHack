// ============================================================================
// 文件：src/Engine/il2cpp_function.h
// ============================================================================
//
// 【这个文件是什么？—— X-Macro 数据表】
//
//   这个文件是一个「纯数据表」，它本身不产生任何代码！
//   它只包含 DO_FUNC(...) 宏调用，每一行代表一个你想要调用的 C# 方法。
//
//   当其他文件 #include 本文件时，会先 #define DO_FUNC 为不同的含义，
//   然后包含本文件，DO_FUNC 就会被展开成不同的代码：
//
//   ┌──────────────┬──────────────────────────────────────────────────────────┐
//   │ 展开位置      │ DO_FUNC 被定义为什么                                      │
//   ├──────────────┼──────────────────────────────────────────────────────────┤
//   │ Engine.h     │ using FindObjectsOfType_t = ...; extern FindObjectsOfType_t FindObjectsOfType; │
//   │              │ → 声明函数指针类型 + extern 声明变量                         │
//   ├──────────────┼──────────────────────────────────────────────────────────┤
//   │ Engine.cpp   │ FindObjectsOfType_t FindObjectsOfType = nullptr;          │
//   │ (变量定义)    │ → 定义全局函数指针变量，初始化为 nullptr                      │
//   ├──────────────┼──────────────────────────────────────────────────────────┤
//   │ Engine.cpp   │ FindObjectsOfType = (FindObjectsOfType_t)GetMethod(...);  │
//   │ (Initialize) │ → 在初始化时通过 GetMethod 获取真实函数地址                   │
//   └──────────────┴──────────────────────────────────────────────────────────┘
//
// 【DO_FUNC 参数说明】
//
//   DO_FUNC(返回类型, 变量名, (参数列表), "程序集名", "命名空间", "类名", "C#方法名")
//
//   参数1: 返回类型     — C# 方法的返回值类型（C++ 中的对应类型）
//   参数2: 变量名       — C++ 中的函数指针变量名（可自定义，避免冲突）
//   参数3: (参数列表)   — C# 方法的参数，用 C++ 类型表示，必须加括号
//   参数4: "程序集名"   — C# 方法所在的 dll 名（不带 .dll 后缀）
//   参数5: "命名空间"   — 类所在的命名空间
//   参数6: "类名"       — 方法所在的类名
//   参数7: "C#方法名"   — ★ 实际的 C# 方法名（必须和 IL2CPP 中的方法名完全一致）
//
//   【为什么参数2和参数7要分开？】
//
//   参数2 是 C++ 变量名，可以自由命名（如 GameObject_get_transform）。
//   参数7 是 IL2CPP 中真实的方法名（如 "get_transform"），GetMethod 按此名搜索。
//
//   如果用同一个名字，比如 GameObject_get_transform，GetMethod 会搜索
//   "GameObject_get_transform" 这个方法名，但 IL2CPP 中不存在这个方法。
//   实际的方法名是 "get_transform"（C# 属性 getter 的命名规则）。
//
//   所以必须分开：
//     DO_FUNC(void*, GameObject_get_transform, (void*), ..., "get_transform")
//                 ↑ C++ 变量名                              ↑ IL2CPP 真实方法名
//
//   【C# 属性 getter/setter 的命名规则】
//
//   C# 属性            → IL2CPP 方法名
//   transform { get; } → "get_transform"
//   layer { get; set;} → "get_layer" / "set_layer"
//   position { get; }  → "get_position"
//
// ============================================================================

// ----------------------------------------------------------------------------
// UnityEngine.Object.FindObjectsOfType(Type type)
// ----------------------------------------------------------------------------
DO_FUNC(Unity_Array*, FindObjectsOfType, (void*), "UnityEngine.CoreModule", "UnityEngine", "Object", "FindObjectsOfType");

// ----------------------------------------------------------------------------
// System.Object.GetType()
// ----------------------------------------------------------------------------
DO_FUNC(void*, GetType, (void*), "mscorlib", "System", "Type", "GetType");




// ----------------------------------------------------------------------------
// System.Runtime.InteropServices.Marshal.PtrToStringAnsi(IntPtr ptr)

//返回值类型：string   ->  C++ 类型：void*

//参数：IntPtr ptr ->  C++ 类型：const char*

//程序集：mscorlib -> c# 程序集名

//命名空间：System.Runtime.InteropServices  -> c# 命名空间

//类名：Marshal -> c# 类名

//方法名：PtrToStringAnsi -> c# 方法名
// ----------------------------------------------------------------------------
DO_FUNC(void*, PtrToStringAnsi, (const char*), "mscorlib", "System.Runtime.InteropServices", "Marshal", "PtrToStringAnsi");



/*
返回值：Camera*  -> C++ 类型：Camera* （注意：这里的 Camera* 是我们在 class.h 中定义的简化版本，不是完整的 Il2CppObject）
定义：GetMainCamera
参数：get_main() 没有参数 → ()
程序集：UnityEngine.CoreModule（标准 Unity 核心模块，Camera 类几乎都在这里）
命名空间：UnityEngine  // Namespace: UnityEngine
类名：Camera   （public sealed class Camera : Behaviour // TypeDefIndex: 2620）
方法：
	// Methods
	[FreeFunctionAttribute] // RVA: 0x129C0 Offset: 0x11DC0 VA: 0x1800129C0
	// RVA: 0xA2BC50 Offset: 0xA2AA50 VA: 0x180A2BC50
	public static Camera get_main() { }
*/
DO_FUNC(Camera*, GetMainCamera,(), "UnityEngine.CoreModule", "UnityEngine", "Camera", "get_main");



/*
返回值：Transform*  -> C++ 类型：Transform* （注意：这里的 Transform* 是我们在 class.h 中定义的简化版本，不是完整的 Il2CppObject）
定义：GameObject_get_transform
参数：get_transform() 没有参数 → ()
程序集：UnityEngine.CoreModule（标准 Unity 核心模块，Camera 类几乎都在这里）
命名空间：UnityEngine  
*/





/*public class Component : Object // TypeDefIndex: 2757
{
	// Properties
	public Transform transform { get; }
	public GameObject gameObject { get; }
	public string tag { get; }

	// Methods

	[FreeFunctionAttribute] // RVA: 0x3A8E0 Offset: 0x39CE0 VA: 0x18003A8E0
	// RVA: 0xA2E240 Offset: 0xA2D040 VA: 0x180A2E240
	public Transform get_transform() { }*/
// GameObject.transform { get; } → 实际方法名: get_transform
// 注意：C++ 变量名用 GameObject_get_transform 避免与 class.h 中的方法名冲突
//获取一个 GameObject 上的 Transform 组件
DO_FUNC(void*, GameObject_get_transform, (void*), "UnityEngine.CoreModule", "UnityEngine", "GameObject", "get_transform");









// ============================================================================
// GameObject 常用方法
// ============================================================================


// GameObject.layer { get; } → 实际方法名: get_layer
DO_FUNC(int, GameObject_get_layer, (void*), "UnityEngine.CoreModule", "UnityEngine", "GameObject", "get_layer");

// GameObject.activeSelf { get; } → 实际方法名: get_activeSelf
DO_FUNC(bool, GameObject_get_activeSelf, (void*), "UnityEngine.CoreModule", "UnityEngine", "GameObject", "get_activeSelf");

// GameObject.activeInHierarchy { get; } → 实际方法名: get_activeInHierarchy
DO_FUNC(bool, GameObject_get_activeInHierarchy, (void*), "UnityEngine.CoreModule", "UnityEngine", "GameObject", "get_activeInHierarchy");

// GameObject.tag { get; } → 实际方法名: get_tag
DO_FUNC(void*, GameObject_get_tag, (void*), "UnityEngine.CoreModule", "UnityEngine", "GameObject", "get_tag");

// GameObject.Find(string name) → 实际方法名: Find
DO_FUNC(void*, GameObject_Find, (void*), "UnityEngine.CoreModule", "UnityEngine", "GameObject", "Find");

// ============================================================================
// Transform 常用方法
// ============================================================================




// Transform.position { get; } → 实际方法名: get_position
// 注意：C++ 变量名用 Transform_get_position 避免与 class.h 中的方法名冲突
// 返回值用 Unity_Vector3（完整定义），不能用 Vector3（DO_FUNC 展开为函数指针类型时需要完整类型）
DO_FUNC(Unity_Vector3, Transform_get_position, (void*), "UnityEngine.CoreModule", "UnityEngine", "Transform", "get_position");



// Transform.rotation { get; } → 实际方法名: get_rotation
DO_FUNC(Unity_Quaternion, Transform_get_rotation, (void*), "UnityEngine.CoreModule", "UnityEngine", "Transform", "get_rotation");

// Transform.lossyScale { get; } → 实际方法名: get_lossyScale
DO_FUNC(Unity_Vector3, Transform_get_lossyScale, (void*), "UnityEngine.CoreModule", "UnityEngine", "Transform", "get_lossyScale");
