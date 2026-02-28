#include "ConfigFile.hpp"

#include "common/helpers/FileHelpers.hpp"
#include "core/Exceptions.hpp"
#include "logging/LoggingFactory.hpp"

#include <star_common/helper/PathHelpers.hpp>

#include <nlohmann/json.hpp>

#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>

using json = nlohmann::json;

boost::mutex star::ConfigFile::mutex = boost::mutex();

std::map<star::Config_Settings, std::string> star::ConfigFile::settings =
    std::map<star::Config_Settings, std::string>();

std::map<std::string, star::Config_Settings> star::ConfigFile::availableSettings = {
    std::pair<std::string, star::Config_Settings>("app_name", star::Config_Settings::app_name),
    std::pair<std::string, star::Config_Settings>("media_directory", star::Config_Settings::mediadirectory),
    std::pair<std::string, star::Config_Settings>("texture_filtering", star::Config_Settings::texture_filtering),
    std::pair<std::string, star::Config_Settings>("texture_anisotropy", star::Config_Settings::texture_anisotropy),
    std::pair<std::string, star::Config_Settings>("frames_in_flight", star::Config_Settings::frames_in_flight),
    std::pair<std::string, star::Config_Settings>("required_device_feature_shader_float64",
                                                  star::Config_Settings::required_device_feature_shader_float64),
    std::make_pair("resolution_x", star::Config_Settings::resolution_x),
    std::make_pair("resolution_y", star::Config_Settings::resolution_y),
    std::make_pair("tmp_dir", star::Config_Settings::tmp_directory)};

void star::ConfigFile::load(const std::string &configFilePath)
{
    if (!file_helpers::FileExists(configFilePath))
    {
        std::ostringstream oss;
        oss << "Provided config file was not found: " << configFilePath << std::endl
            << "A default config file should have been created with the starlight app builder project" << std::endl;

        STAR_THROW(oss.str());
    }

    json configJson;

    try
    {
        std::fstream configStream(configFilePath);
        configStream >> configJson;
    }
    catch (std::exception &e)
    {
        std::ostringstream oss;

        oss << "Error reading config file: ";
        oss << e.what();
        STAR_THROW(oss.str());
    }

    for (auto &setting : availableSettings)
    {
        if (configJson.contains(setting.first))
        {
            settings[setting.second] = configJson[setting.first].get<std::string>();
        }
        else
        {
            // get default value for this setting
            switch (setting.second)
            {
            case Config_Settings::texture_filtering:
                settings[setting.second] = "linear";
                break;
            case Config_Settings::texture_anisotropy:
                settings[setting.second] = "0";
                break;
            case Config_Settings::frames_in_flight:
                settings[setting.second] = "2";
                break;
            case Config_Settings::tmp_directory: {
                const auto path = star::file_helpers::GetExecutableDirectory() / "tmp";
                settings[setting.second] = path.string();

                if (!std::filesystem::exists(path)){
                    core::logging::info("Creating temporary data directory: " + path.string()); 
                    std::filesystem::create_directories(path);
                }
                break;
            }
            default:
                STAR_THROW("Setting not found and has no available default: " + setting.first);
            }
        }
    }
}

std::string star::ConfigFile::getSetting(Config_Settings setting)
{
    boost::unique_lock<boost::mutex> lock = boost::unique_lock<boost::mutex>(mutex);

    auto settingsRecord = settings.find(setting);

    if (settingsRecord != settings.end())
    {
        return settingsRecord->second;
    }

    std::string settingName;
    switch (setting)
    {
    case (Config_Settings::app_name):
        settingName = "App_Name";
        break;
    case (Config_Settings::frames_in_flight):
        settingName = "Frames_in_flight";
        break;
    case (Config_Settings::mediadirectory):
        settingName = "mediadirectory";
        break;
    case (Config_Settings::required_device_feature_shader_float64):
        settingName = "Shader_float64";
        break;
    case (Config_Settings::resolution_x):
        settingName = "resolution_x";
        break;
    case (Config_Settings::resolution_y):
        settingName = "resolution_y";
        break;
    case (Config_Settings::texture_anisotropy):
        settingName = "texture_anisotropy";
        break;
    case (Config_Settings::texture_filtering):
        settingName = "texture_filtering";
        break;
    case (Config_Settings::tmp_directory):
        settingName = "tmp_directory";
        break;
    default:
        settingName = "UNKNOWN";
        break;
    }

    std::ostringstream oss;
    oss << "Setting not found: " << settingName << std::endl;
    STAR_THROW(oss.str());
}