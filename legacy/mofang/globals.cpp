#include "globals.h"


DWORD picker_flags = ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaPreview;
float tab_size = 0.f;
float arrow_roll = 0.f;
bool tab_opening = true;
int rotation_start_index;
bool Dcheckbox = false;
bool Dcheckbox_3D = false;
bool Bonecheckbox = false;



bool IsF1 = false;

bool IsF2 = false;



float boxtk = 1.f;
float boxtk_3D = 1.f;
float hptk = 1.f;
float hdtk = 1.f;
float bonetk = 1.f;



Xmm12Data* xmmData;

AActor* player_ = nullptr;

AActor* Enemy_lock = nullptr;


AActor* Enemy_ = nullptr;




AGameStateBase* First_Initialization_Check = nullptr;

AGameStateBase* First_Initialization_Check_ = nullptr;


//
//bool Is_AtuoConfig = false;
//bool Is_First_Auto = false;
//
//bool Is_First_Auto_check = false;

bool isAutoConfig = false;
bool isFirstAuto = false;
bool isFirstAutoCheck = false;



int choose_aim = 0;
int choose_sx = 0;

int offsetX = 20;
int offsetY = 10;
int smallerWidth = 202;
int smallerHeight = 364;


int select_aim = 0;
int select_aim_location = 0;



int select_sx_location = 0;

int select_sxc_location = 0;






// 功能开关数组
bool Flogs[6] = { false, false, false, false, false };
// 功能名称数组
const char* Flogss[6] = { u8"自瞄", u8"透视", u8"雷达", u8"距离", u8"骨骼" };




float box2D_colorN[4] = { 0.3f, 0.7f, 1.0f, 1.0f };
float box3D_colorN[4] = { 0.3f, 0.7f, 1.0f, 1.0f };

float SnapLine_colorN[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
float Distance_colorN[4] = { 0.8f, 0.2f, 0.8f, 1.0f };
float LosLine_colorN[4] = { 1.0f, 0.5f, 0.0f, 1.0f };
float Name_colorN[4] = { 1.0f, 0.8f, 0.0f, 1.0f };

float Range_color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };



float Radar_Player_colorN[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
float Radar_Line_colorN[4] = { 0.0f, 0.0f, 0.0f, 1.0f };




float Bone_Unobstructed_ColorN[4] = { 0.5f, 1.0f, 0.5f, 1.0f };

float AimBot_unObstructed_ColorN[4] = { 1.0f, 1.0f, 0.0f, 1.0f };
float Bone_Occlusion_ColorN[4] = { 0.5f, 0.0f, 0.5f, 1.0f };




float Radar_Occlusion_ColorN[4] = { 0.5f, 0.0f, 0.5f, 1.0f };

float Radar_unObstructed_ColorN[4] = { 0.5f, 1.0f, 0.5f, 1.0f };





float Debug_Bone_Name_ColorN[4] = { 0.0f, 1.0f, 0.5f, 1.0f };
float Debug_Class_Void_ColorN[4] = { 0.8f, 0.2f, 0.8f, 1.0f };
float Debug_Bone_Count_ColorN[4] = { 0.5f, 0.5f, 1.0f, 1.0f };






int key = 162;