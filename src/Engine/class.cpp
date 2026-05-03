#include "class.h"
#include "Engine.h"       // 全局函数指针声明在此


// ============================================================================
// Transform 方法实现
// ============================================================================

Unity_Vector3 Transform::GetPosition()
{
    // 调用全局函数指针（由 il2cpp_function.h X-Macro 生成）
    if (!::Transform_get_position) return { 0, 0, 0 }; // 先检查函数指针是否有效，如果无效返回默认坐标
    return ::Transform_get_position(this);// 调用绑定的 C# 方法指针获取位置 
	// 注意：这里返回的是 Unity_Vector3，代表 Vector3，因为我们在 Engine.h 中声明的 GetPosition 返回类型是 Unity_Vector3。
}

// ============================================================================
// GameObject 方法实现
// ============================================================================

Transform* GameObject::GetTransform()
{
    if (!::GameObject_get_transform) return nullptr;
    return reinterpret_cast<Transform*>(::GameObject_get_transform(this)); // reinterpret_cast 用于将 void* 转换为 Transform*
	// 注意：这里返回的是 void*，代表 Transform*，因为我们在 Engine.h 中声明的 GetTransform 返回类型是 void*。
}

// ============================================================================
// Camera 方法实现
// ============================================================================

bool Camera::WorldToScreen(Vector3 position, Vector2& Point)
{
	uint8_t* temp01 = *(uint8_t**)(((uint8_t*)this) + 0x10);
	D3DXMATRIX* Matrix = (D3DXMATRIX*)(temp01 + 0xDC);

	float Z = Matrix->_14 * position.X + Matrix->_24 * position.Y + Matrix->_34 * position.Z + Matrix->_44;

	if (Z > 0)
	{
		float X = Matrix->_11 * position.X + Matrix->_21 * position.Y + Matrix->_31 * position.Z + Matrix->_41;
		float Y = Matrix->_12 * position.X + Matrix->_22 * position.Y + Matrix->_32 * position.Z + Matrix->_42;

		float NdcX = X / Z;
		float NdcY = Y / Z;

		Point.X = (NdcX + 1.f) / 2.f * ImGui::GetIO().DisplaySize.x;
		Point.Y = (1.f - (NdcY + 1.f) / 2.f) * ImGui::GetIO().DisplaySize.y;

		return true;
	}


	return false;
}
