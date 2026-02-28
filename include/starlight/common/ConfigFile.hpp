#pragma once

#include "enums/Enums.hpp"

#include <boost/thread/mutex.hpp>

#include <string> 
#include <map>

namespace star {
    class ConfigFile {
    public:
        ConfigFile() = default; 
        ~ConfigFile() = default;

        static void load(const std::string& configFilePath); 

        static std::string getSetting(Config_Settings setting); 

    private:
        static boost::mutex mutex; 
        static std::map<Config_Settings, std::string> settings;

        static std::map<std::string, Config_Settings> availableSettings; 
    };
}