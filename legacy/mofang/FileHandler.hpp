#pragma once

#include <fstream>
#include "json.hpp"
#include "base.h"
#include "globals.h"
#include <cstdio>
#include <filesystem>
#include <string>



class FileHandler {
public:
    enum class FileStatus {
        NOT_EXISTS,
        EMPTY,
        NON_EMPTY_DELETED_SUCCESSFULLY,
        NON_EMPTY_DELETION_FAILED
    };

    static FileStatus removeFile(const std::string& filename);

    static bool loadConfigAuto(const std::string& filename, bool& isAutoConfig, bool& isFirstAutoCheck);
    static void saveConfigAuto(const std::string& filename, bool isAutoConfig, bool isFirstAutoCheck);

    static void SaveConfig(const std::string& filename);
    static void LoadConfig(const std::string& filename);

    static bool fileExists(const std::string& filename);
    static bool isFileEmpty(const std::string& filename);
};