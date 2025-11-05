#include "ConfigFile.hpp"

#include "FileHelpers.hpp"

#include <iostream> 

#include <sstream>
#include <map> 
#include <memory> 
#include <assert.h>
#include <fstream>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

boost::mutex star::ConfigFile::mutex = boost::mutex(); 

std::map<star::Config_Settings, std::string> star::ConfigFile::settings = std::map<star::Config_Settings, std::string>(); 

std::map<std::string, star::Config_Settings> star::ConfigFile::availableSettings = {
            std::pair<std::string, star::Config_Settings>("app_name", star::Config_Settings::app_name),
            std::pair<std::string, star::Config_Settings>("media_directory", star::Config_Settings::mediadirectory),
            std::pair<std::string, star::Config_Settings>("texture_filtering", star::Config_Settings::texture_filtering),
            std::pair<std::string, star::Config_Settings>("texture_anisotropy", star::Config_Settings::texture_anisotropy),
            std::pair<std::string, star::Config_Settings>("frames_in_flight", star::Config_Settings::frames_in_flight),
            std::pair<std::string, star::Config_Settings>("required_device_feature_shader_float64", star::Config_Settings::required_device_feature_shader_float64),
            std::make_pair("resolution_x", star::Config_Settings::resolution_x), 
            std::make_pair("resolution_y", star::Config_Settings::resolution_y)
};

void star::ConfigFile::load(const std::string& configFilePath) {
    if (!file_helpers::FileExists(configFilePath)){
        throw std::runtime_error("No config file found"); 
    }
    
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
                case Config_Settings::frames_in_flight:
                    settings[setting.second] = "2"; 
				default:
					throw std::runtime_error("Setting not found and has no available default: " + setting.first);
			}
		}
    }
}

std::string star::ConfigFile::getSetting(Config_Settings setting) {
    boost::unique_lock<boost::mutex> lock = boost::unique_lock<boost::mutex>(mutex);
    
    auto settingsRecord = settings.find(setting);

    if (settingsRecord != settings.end()) {
        return settingsRecord->second;
    }
    throw std::runtime_error("Setting not found " + setting);
}