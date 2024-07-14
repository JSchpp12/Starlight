#pragma once

#include "FileHelpers.hpp"
#include "Enums.hpp"

#include <iostream> 
#include <string> 
#include <sstream>
#include <map> 
#include <memory> 
#include <assert.h>
#include <fstream>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace star {
    class ConfigFile {
    public:
        ConfigFile() = default; 
        ~ConfigFile() = default;

        static void load(const std::string& configFilePath); 

        static std::string getSetting(Config_Settings setting); 

    private:
        static std::map<Config_Settings, std::string> settings;

        static std::map<std::string, Config_Settings> availableSettings; 
    };
}