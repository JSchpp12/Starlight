#include "ConfigFile.hpp"

std::map<star::Config_Settings, std::string> star::ConfigFile::settings = std::map<star::Config_Settings, std::string>(); 

std::map<std::string, star::Config_Settings> star::ConfigFile::availableSettings = {
            std::pair<std::string, star::Config_Settings>("media_directory", star::Config_Settings::mediadirectory),
            std::pair<std::string, star::Config_Settings>("texture_filtering", star::Config_Settings::texture_filtering),
            std::pair<std::string, star::Config_Settings>("texture_anisotropy", star::Config_Settings::texture_anisotropy),
};

void star::ConfigFile::load(const std::string& configFilePath) {
    assert(FileHelpers::FileExists(configFilePath));

    json configJson;

    try{
        std::fstream configStream(configFilePath);
        configStream >> configJson;
    }catch(std::exception& e){
		std::cerr << "Error reading config file: " << e.what() << std::endl;
	}

    for (auto& setting : availableSettings) {
        if (configJson.contains(setting.first)) {
			settings[setting.second] = configJson[setting.first].get<std::string>();
		} else {
            //get default value for this setting
            switch(setting.second){
				case Config_Settings::texture_filtering:
					settings[setting.second] = "linear";
					break;
                case Config_Settings::texture_anisotropy:
                    settings[setting.second] = "0";
					break;
				default:
					throw std::runtime_error("Setting not found and has no available default: " + setting.first);
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