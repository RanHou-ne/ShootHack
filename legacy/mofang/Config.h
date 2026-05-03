#ifndef CONFIG_H
#define CONFIG_H
#include <fstream>
#include "json.hpp"
#include "base.h"
#include "globals.h"
#include <cstdio>
#include <filesystem>
#include <string>

void SaveConfig(const string& filename);

void LoadConfig(const string& filename);

bool loadConfigAuto(const std::string& filename, bool& isAutoConfig, bool& isFirstAutoCheck);
//bool LoadConfig_Auto(const std::string& filename);
void saveConfigAuto(const std::string& filename, bool isAutoConfig, bool isFirstAutoCheck);
//void SaveConfig_Auto(const std::string& filename);


int RemoveFiile(const string& filename);

bool fileExists(const string& filename);

bool isFileEmpty(const string& filename);





class FileHandler {
public:
    enum class FileStatus {
        NOT_EXISTS,                     // 文件不存在
        EMPTY,                          // 文件为空
        NON_EMPTY_DELETED_SUCCESSFULLY, // 非空文件删除成功
        NON_EMPTY_DELETION_FAILED       // 非空文件删除失败
    };

    static FileStatus RemoveFiile(const std::string& filename);





};

#endif //!CONFIG_H











//#ifndef CONFIG_H
//#define CONFIG_H
//
//#include <string>
//#include <json.hpp>
//#include <filesystem>
//#include <iostream>
//#include <fstream>
//#include "base.h"
//#include "globals.h"
//#include <cstdio>
//#include <string>
//
//
//class FileHandler {
//public:
//    enum class FileStatus {
//        NOT_EXISTS,
//        EMPTY,
//        NON_EMPTY_DELETED_SUCCESSFULLY,
//        NON_EMPTY_DELETION_FAILED
//    };
//
//    static FileStatus removeFile(const std::string& filename);
//
//    static bool loadConfigAuto(const std::string& filename, bool& isAutoConfig, bool& isFirstAutoCheck);
//    static void saveConfigAuto(const std::string& filename, bool isAutoConfig, bool isFirstAutoCheck);
//
//    static void SaveConfig(const std::string& filename);
//    static void LoadConfig(const std::string& filename);
//
//    static bool fileExists(const std::string& filename);
//    static bool isFileEmpty(const std::string& filename);
//};
//#endif //!CONFIG_H