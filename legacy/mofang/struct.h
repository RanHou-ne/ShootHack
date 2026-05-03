#ifndef STRUCT_H
#define STRUCT_H

#include"base.h"
#include<vector>
#include <cmath>


struct Vector2
{
	float X = 0 , Y = 0 ;




};
struct Vector3 :public Vector2
{

	Vector3() {};

	Vector3(float A, float B, float C)
	{
		X = A;
		Y = B;
		Z = C;
	};

	float Z = 0 ;


};
struct Vector4 :public Vector3
{
	float W = 0 ;
};


struct Vector3f
{
	float X = 0, Y = 0, Z = 0;
};





int getObjectDistance(Vector3 selfCoord, Vector3 objectCoord);


struct FMatrix
{
	float _11, _12, _13, _14;//矩阵4*4元素
	float _21, _22, _23, _24;
	float _31, _32, _33, _34;
	float _41, _42, _43, _44;

	FMatrix operator*(const FMatrix& other);
};




struct Xmm12Data {
	float value1;
	float value2;
	float value3;
};



struct BoneIdx
{
	int head, neck_01, spine_03, spine_02, spine_01, pelvis; //头部到盆骨骨骼索引
	int hand_l, lowerarm_l, upperarm_l, clavicle_l, clavicle_r, upperarm_r, lowerarm_r, hand_r; //手臂骨骼索引
	int ball_l, foot_l, calf_l, thigh_l, thigh_r, calf_r, foot_r, ball_r, Root; //腿部骨骼索引
	int index_03_r, midd1e_03_r, thumb_03_r, pinky_03_r, index_03_l, midd1e_03_l, thumb_03_l, pinky_03_l, EyeBone_r, EyeBone_l, unrealEye_R, unrealEye_L, eye_r, eye_l;
};





struct FLinearColor :public Vector4 //射线检测call函数参数
{
};



struct FRotator  //旋转
{
	float Pitch = 0;
	float Yaw = 0;
	float Roll = 0;
};



struct FHitResult
{
	char pa_00[0x0088]; //射线检测结果填充
};




struct  BoneName
{
	uint32_t Name; //骨骼名称Name索引
	int A;
	int B;//未知用途偏移
};




struct FTransform_
{
	struct Vector4 Rotation; // 0x0000(0x0010)
	struct Vector3 Translation; // 0x0010(0x000C)
	char pad_1C[0x4]; // 0x001C(0x0004)
	struct Vector3 Scale3D; // 0x0020(0x000C)
	char pad_2C[0x4]; // 0x002C(0x0004)
};




struct FTransform //30
{
	//3*3矩阵变换
	Vector4 Rotation;//旋转
	Vector4  Translation;//位置
	Vector4 Scale3D;//缩放
	FMatrix ToMatrixWithScale();
};



struct FNameEntry
{
	uint16_t bIsWide : 1;
	static constexpr uint32_t ProbeHashBits = 5;
	uint16_t LovercaseProbeHash : ProbeHashBits;
	uint16_t Len : 10;

	union
	{
		char AnsiName[1024];
		wchar_t WideName[1024];
	};
};




#endif //STRUCT_H