#include "ConfigFile.hpp"

std::map<star::Config_Settings, std::string> star::ConfigFile::settings = std::map<star::Config_Settings, std::string>(); 

std::map<std::string, star::Config_Settings> star::ConfigFile::availableSettings = {
            std::pair<std::string, star::Config_Settings>("mediadirectory", star::Config_Settings::mediadirectory)
};

void star::ConfigFile::load(const std::string& configFilePath) {
    auto contents = FileHelpers::ReadFile(configFilePath, false);
    std::istringstream stream(contents);

    std::string line;
    while (std::getline(stream, line)) {
        std::istringstream lineStream(line);
        std::string key;
        if (std::getline(lineStream, key, '=')) {
            std::string value;
            if (std::getline(lineStream, value)) {
                auto settingsMatch = availableSettings.find(key);
                if (settingsMatch != availableSettings.end()) {
                    ConfigFile::settings.insert(std::pair<Config_Settings, std::string>(settingsMatch->second, value));
                }
            }
        }
    }
}

std::string star::ConfigFile::getSetting(Config_Settings setting) {
    auto settingsRecord = settings.find(setting);

    if (settingsRecord != settings.end()) {
        return settingsRecord->second;
    }
    throw std::runtime_error("Setting not found " + setting);
}