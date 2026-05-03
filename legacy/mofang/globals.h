
#include "D3DX11tex.h"
#include "imgui/imgui.h"
#include"class.h"

#pragma once

// Declare global variables

extern DWORD picker_flags;
extern float tab_size;
extern float arrow_roll;
extern bool tab_opening;
extern int rotation_start_index;
extern bool Dcheckbox;
extern bool Dcheckbox_3D;
extern bool Bonecheckbox;


//
//extern bool Is_AtuoConfig;
//
//extern bool Is_First_Auto;
//
//extern bool Is_First_Auto_check;



extern bool isAutoConfig;
extern bool isFirstAuto ;
extern bool isFirstAutoCheck;


extern bool IsF1;

extern bool IsF2;


extern float boxtk;
extern float boxtk_3D;
extern float hptk;
extern float hdtk;
extern float bonetk;

extern int select_aim;

extern int select_aim_location;

extern int select_sx_location;

extern int select_sxc_location;


extern int choose_aim;


extern int choose_sx;

extern Xmm12Data* xmmData;

extern AActor* player_;

extern AActor* Enemy_lock;

extern AActor* Enemy_;

extern AGameStateBase* First_Initialization_Check;

extern AGameStateBase* First_Initialization_Check_;




extern int offsetX;
extern int offsetY;
extern int smallerWidth;
extern int smallerHeight;

extern bool Flogs[];
extern const char* Flogss[];

extern float box2D_colorN[];
extern float box3D_colorN[];



extern float SnapLine_colorN[];
extern float Distance_colorN[];
extern float LosLine_colorN[];
extern float Name_colorN[];




extern float Radar_Player_colorN[] ;
extern float Radar_Line_colorN[];





extern float Range_color[];
extern float Bone_Unobstructed_ColorN[];

extern float AimBot_unObstructed_ColorN[];


extern float Radar_Occlusion_ColorN[];
extern float Radar_unObstructed_ColorN[] ;








extern float Bone_Occlusion_ColorN[];
extern float Debug_Bone_Name_ColorN[];
extern float Debug_Class_Void_ColorN[];
extern float Debug_Bone_Count_ColorN[];
extern int key;