#include "FileHandler.hpp"

namespace fs = std::filesystem;

// 自动加载配置文件
bool FileHandler::loadConfigAuto(const std::string& filename, bool& isAutoConfig, bool& isFirstAutoCheck) {
    std::ifstream file(filename);
    if (!file) {
        std::cout << "无法打开配置文件" << std::endl;
        return false;
    }
    nlohmann::json j;
    try {
        file >> j;
    }
    catch (const std::exception& e) {
        std::cout << "解析 JSON 配置文件出错: " << e.what() << std::endl;
        file.close();
        return false;
    }
    if (j.find("Is_AtuoConfig") != j.end() && isFirstAutoCheck) {
        isAutoConfig = j["Is_AtuoConfig"].get<bool>();
    }
    if (j.find("Is_First_Auto_check") != j.end()) {
        isFirstAutoCheck = j["Is_First_Auto_check"].get<bool>();
    }
    else {
        std::cout << "JSON 配置文件缺少 'Is_AtuoConfig' 字段" << std::endl;
        file.close();
        return false;
    }
    file.close();
    return true;
}

// 自动保存配置文件
void FileHandler::saveConfigAuto(const std::string& filename, bool isAutoConfig, bool isFirstAutoCheck) {
    nlohmann::json j;
    std::ofstream file(filename);
    if (!file) {
        std::cout << "无法创建配置文件" << std::endl;
        return;
    }
    j["Is_AtuoConfig"] = isAutoConfig;
    j["Is_First_Auto_check"] = isFirstAutoCheck;
    file << j.dump(4);
    file.close();
}

void FileHandler::SaveConfig(const std::string& filename) {
    nlohmann::json j;
    std::ofstream file(filename);
    if (!file) {
        std::cout << "无法创建配置文件" << std::endl;
        return;
    }

    // 将 Options 结构体中的配置项保存到 JSON 对象中
    j["Open_menu"] = Options.Open_menu;
    j["boneOn"] = Options.boneOn;
    j["Enemies_Freeze"] = Options.Enemies_Freeze;
    j["OpenAimbot"] = Options.OpenAimbot;
    j["BulletAim"] = Options.BulletAim;
    j["TGSoAS"] = Options.TGSoAS;
    j["MemoryAimbot"] = Options.MemoryAimbot;
    j["Missed_shot"] = Options.Missed_shot;
    j["DrawSnapLine"] = Options.DrawSnapLine;
    j["DrawBox2D"] = Options.DrawBox2D;
    j["DrawBlood"] = Options.DrawBlood;
    j["DrawRadar"] = Options.DrawRadar;
    j["DrawDistance"] = Options.DrawDistance;
    j["DrawLosLine"] = Options.DrawLosLine;
    j["DrawBox3D"] = Options.DrawBox3D;
    j["DrawRange"] = Options.DrawRange;
    j["DrawCharacterName"] = Options.DrawCharacterName;
    j["MemoryAmmon_num"] = Options.MemoryAmmon_num;
    j["Filtering_out_death"] = Options.Filtering_out_death;
    j["Debug_Bone_Name"] = Options.Debug_Bone_Name;
    j["Debug_Bone_Name_Choose"] = Options.Debug_Bone_Name_Choose;
    j["Debug_Bone_count"] = Options.Debug_Bone_count;
    j["Debug_Class_Void"] = Options.Debug_Class_Void;
    j["Debug_Class_ID"] = Options.Debug_Class_ID;
    j["Debug_character_name"] = Options.Debug_character_name;
    j["Debug_character_name_ALL"] = Options.Debug_character_name_ALL;
    j["NoRecoil"] = Options.NoRecoil;
    j["NoCameraShake"] = Options.NoCameraShake_;
    j["SwayLeftAndRight"] = Options.SwayLeftAndRight_;
    j["SightDiffusion"] = Options.SightDiffusion_;
    j["RapidFire"] = Options.RapidFire_;
    j["Ammon_num"] = Options.Ammon_num;
    j["Ammon_Clip_num"] = Options.Ammon_Clip_num;
    j["Aim_Range"] = Options.Aim_Range;
    j["RadarPos"]["X"] = Options.RadarPosX;
    j["RadarPos"]["Y"] = Options.RadarPosY;
    j["RadarSize"] = Options.RadarSize;
    j["RadarSizeR"] = Options.RadarSizeR;
    j["Can_DrawDistance"] = Options.Can_DrawDistance;
    j["LosLine_size"] = Options.LosLine_size;
    j["Radar_Player_size"] = Options.Radar_Player_size;
    j["Radar_Enemy_size"] = Options.Radar_Enemy_size;
    j["Radar_scaling"]["X"] = Options.Radar_scaling_X;
    j["Radar_scaling"]["Y"] = Options.Radar_scaling_Y;
    j["threshold"] = Options.threshold;
    j["RainbowText"] = Options.RainbowText;
    j["Radar_scaling"]["X"] = Options.Radar_scaling_X;
    j["Radar_scaling"]["Y"] = Options.Radar_scaling_Y;
    j["HSR_"] = Options.HSR_;
    j["HR_"] = Options.HR_;
    j["KS"] = Options.KS;
    j["TNBF"] = Options.TNBF;
    j["TNBFZ"] = Options.TNBFZ;
    j["TNBFZH"] = Options.TNBFZH;

    j["Auto_Read_Config"] = Options.Auto_Read_Config;




    j["box2D_colorN"] = box2D_colorN[4];
    j["box3D_colorN"] = box3D_colorN[4];
    j["SnapLine_colorN"] = SnapLine_colorN[4];
    j["Distance_colorN"] = Distance_colorN[4];
    j["LosLine_colorN"] = LosLine_colorN[4];
    j["Name_colorN"] = Name_colorN[4];
    j["Range_color"] = Range_color[4];
    j["Radar_Player_colorN"] = Radar_Player_colorN[4];
    j["Radar_Line_colorN"] = Radar_Line_colorN[4];
    j["Bone_Unobstructed_ColorN"] = Bone_Unobstructed_ColorN[4];
    j["AimBot_unObstructed_ColorN"] = AimBot_unObstructed_ColorN[4];
    j["Bone_Occlusion_ColorN"] = Bone_Occlusion_ColorN[4];
    j["Radar_Occlusion_ColorN"] = Radar_Occlusion_ColorN[4];
    j["Radar_unObstructed_ColorN"] = Radar_unObstructed_ColorN[4];
    j["Debug_Bone_Name_ColorN"] = Debug_Bone_Name_ColorN[4];
    j["Debug_Class_Void_ColorN"] = Debug_Class_Void_ColorN[4];
    j["Debug_Bone_Count_ColorN"] = Debug_Bone_Count_ColorN[4];
    j["key"] = key;
    j["choose_aim"] = choose_aim;
    j["choose_sx"] = choose_sx;
    j["offset"]["X"] = offsetX;
    j["offset"]["Y"] = offsetY;
    j["smallerWidth"] = smallerWidth;
    j["smallerHeight"] = smallerHeight;
    j["select_aim"] = select_aim;
    j["select_aim_location"] = select_aim_location;
    j["select_sx_location"] = select_sx_location;
    j["select_sxc_location"] = select_sxc_location;

    j["Dcheckbox"] = Dcheckbox;
    j["Dcheckbox_3D"] = Dcheckbox_3D;
    j["Bonecheckbox"] = Bonecheckbox;
    j["boxtk"] = boxtk;
    j["hptk"] = hptk;
    j["hdtk"] = hdtk;
    j["bonetk"] = bonetk;
    j["boxtk_3D"] = boxtk_3D;



    file << j.dump(4);
    file.close();
}

void FileHandler::LoadConfig(const std::string& filename) {
    std::ifstream file(filename);
    nlohmann::json j;
    file >> j;
    Options.Open_menu = j["Open_menu"].get<bool>();
    Options.boneOn = j["boneOn"].get<bool>();
    Options.Enemies_Freeze = j["Enemies_Freeze"].get<bool>();
    Options.OpenAimbot = j["OpenAimbot"].get<bool>();
    Options.BulletAim = j["BulletAim"].get<bool>();
    Options.TGSoAS = j["TGSoAS"].get<bool>();
    Options.MemoryAimbot = j["MemoryAimbot"].get<bool>();
    Options.Missed_shot = j["Missed_shot"].get<bool>();
    Options.DrawSnapLine = j["DrawSnapLine"].get<bool>();
    Options.DrawBox2D = j["DrawBox2D"].get<bool>();
    Options.DrawBlood = j["DrawBlood"].get<bool>();
    Options.DrawRadar = j["DrawRadar"].get<bool>();
    Options.DrawDistance = j["DrawDistance"].get<bool>();
    Options.DrawLosLine = j["DrawLosLine"].get<bool>();
    Options.DrawBox3D = j["DrawBox3D"].get<bool>();
    Options.DrawRange = j["DrawRange"].get<bool>();
    Options.DrawCharacterName = j["DrawCharacterName"].get<bool>();
    Options.MemoryAmmon_num = j["MemoryAmmon_num"].get<bool>();
    Options.Filtering_out_death = j["Filtering_out_death"].get<bool>();
    Options.Debug_Bone_Name = j["Debug_Bone_Name"].get<bool>();
    Options.Debug_Bone_Name_Choose = j["Debug_Bone_Name_Choose"].get<bool>();
    Options.Debug_Bone_count = j["Debug_Bone_count"].get<bool>();
    Options.Debug_Class_Void = j["Debug_Class_Void"].get<bool>();
    Options.Debug_Class_ID = j["Debug_Class_ID"].get<bool>();
    Options.Debug_character_name = j["Debug_character_name"].get<bool>();
    Options.Debug_character_name_ALL = j["Debug_character_name_ALL"].get<bool>();
    Options.NoRecoil = j["NoRecoil"].get<bool>();
    Options.NoCameraShake_ = j["NoCameraShake"].get<float>();
    Options.SwayLeftAndRight_ = j["SwayLeftAndRight"].get<float>();
    Options.SightDiffusion_ = j["SightDiffusion"].get<float>();
    Options.RapidFire_ = j["RapidFire"].get<float>();
    Options.Ammon_num = j["Ammon_num"].get<int>();
    Options.Ammon_Clip_num = j["Ammon_Clip_num"].get<int>();
    Options.Aim_Range = j["Aim_Range"].get<float>();
    Options.RadarPosX = j["RadarPos"]["X"].get<float>();
    Options.RadarPosY = j["RadarPos"]["Y"].get<float>();
    Options.RadarSize = j["RadarSize"].get<float>();
    Options.RadarSizeR = j["RadarSizeR"].get<float>();
    Options.Can_DrawDistance = j["Can_DrawDistance"].get<int>();
    Options.LosLine_size = j["LosLine_size"].get<int>();
    Options.Radar_Player_size = j["Radar_Player_size"].get<int>();
    Options.Radar_Enemy_size = j["Radar_Enemy_size"].get<int>();
    Options.Radar_scaling_X = j["Radar_scaling"]["X"].get<float>();
    Options.Radar_scaling_Y = j["Radar_scaling"]["Y"].get<float>();
    Options.threshold = j["threshold"].get<float>();
    Options.RainbowText = j["RainbowText"].get<bool>();

    Options.HSR_ = j["HSR_"].get<bool>();
    Options.HR_ = j["HR_"].get<bool>();
    Options.KS = j["KS"].get<float>();
    Options.TNBF = j["TNBF"].get<float>();
    Options.TNBFZ = j["TNBFZ"].get<float>();
    Options.TNBFZH = j["TNBFZH"].get<float>();

    Options.Auto_Read_Config = j["Auto_Read_Config"].get<bool>();




    box2D_colorN[4] = j["box2D_colorN"].get<float>();
    box3D_colorN[4] = j["box3D_colorN"].get<float>();
    SnapLine_colorN[4] = j["SnapLine_colorN"].get<float>();
    Distance_colorN[4] = j["Distance_colorN"].get<float>();
    LosLine_colorN[4] = j["LosLine_colorN"].get<float>();
    Name_colorN[4] = j["Name_colorN"].get<float>();
    Range_color[4] = j["Range_color"].get<float>();
    Radar_Player_colorN[4] = j["Radar_Player_colorN"].get<float>();
    Radar_Line_colorN[4] = j["Radar_Line_colorN"].get<float>();
    Bone_Unobstructed_ColorN[4] = j["Bone_Unobstructed_ColorN"].get<float>();
    AimBot_unObstructed_ColorN[4] = j["AimBot_unObstructed_ColorN"].get<float>();
    Bone_Occlusion_ColorN[4] = j["Bone_Occlusion_ColorN"].get<float>();
    Radar_Occlusion_ColorN[4] = j["Radar_Occlusion_ColorN"].get<float>();
    Radar_unObstructed_ColorN[4] = j["Radar_unObstructed_ColorN"].get<float>();
    Debug_Bone_Name_ColorN[4] = j["Debug_Bone_Name_ColorN"].get<float>();
    Debug_Class_Void_ColorN[4] = j["Debug_Class_Void_ColorN"].get<float>();
    Debug_Bone_Count_ColorN[4] = j["Debug_Bone_Count_ColorN"].get<float>();
    key = j["key"].get<int>();






    choose_aim = j["choose_aim"].get<int>();
    choose_sx = j["choose_sx"].get<int>();
    offsetX = j["offset"]["X"].get<int>();
    offsetY = j["offset"]["Y"].get<int>();
    smallerWidth = j["smallerWidth"].get<int>();
    smallerHeight = j["smallerHeight"].get<int>();
    select_aim = j["select_aim"].get<int>();
    select_aim_location = j["select_aim_location"].get<int>();
    select_sx_location = j["select_sx_location"].get<int>();
    select_sxc_location = j["select_sxc_location"].get<int>();
    Dcheckbox = j["Dcheckbox"].get<bool>();
    Dcheckbox_3D = j["Dcheckbox_3D"].get<bool>();
    Bonecheckbox = j["Bonecheckbox"].get<bool>();
    boxtk = j["boxtk"].get<float>();
    boxtk_3D = j["boxtk_3D"].get<float>();
    hptk = j["hptk"].get<float>();
    hdtk = j["hdtk"].get<float>();
    bonetk = j["bonetk"].get<float>();

    file.close();
}

bool FileHandler::fileExists(const std::string& filename) {
    return fs::exists(filename);
}

bool FileHandler::isFileEmpty(const std::string& filename) {
    std::ifstream file(filename);
    return file.peek() == std::ifstream::traits_type::eof();
}

FileHandler::FileStatus FileHandler::removeFile(const std::string& filename) {
    if (!fileExists(filename)) {
        std::cout << "文件不存在" << std::endl;
        return FileStatus::NOT_EXISTS;
    }
    else {
        std::cout << "文件存在";
        if (isFileEmpty(filename)) {
            std::cout << "文件为空" << std::endl;
            return FileStatus::EMPTY;
        }
        else {
            std::cout << "文件不为空";
            try {
                if (fs::remove(filename)) {
                    std::cout << "删除成功" << std::endl;
                    return FileStatus::NON_EMPTY_DELETED_SUCCESSFULLY;
                }
                else {
                    std::cout << "删除失败" << std::endl;
                    return FileStatus::NON_EMPTY_DELETION_FAILED;
                }
            }
            catch (const fs::filesystem_error& e) {
                std::cerr << "删除文件时出错: " << e.what() << std::endl;
                return FileStatus::NON_EMPTY_DELETION_FAILED;
            }
        }
    }
}