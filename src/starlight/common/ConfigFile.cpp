#include "ConfigFile.hpp"

#include "common/ConfigReader.hpp"

#include "common/helpers/FileHelpers.hpp"
#include "core/Exceptions.hpp"
#include "logging/LoggingFactory.hpp"

#include <nlohmann/json.hpp>
#include <optional>

using json = nlohmann::json;

std::map<star::Config_Settings, std::string> star::ConfigFile::settings = {};

std::map<std::string, star::Config_Settings> star::ConfigFile::availableSettings = {
    std::pair<std::string, star::Config_Settings>("app_name", star::Config_Settings::app_name),
    std::pair<std::string, star::Config_Settings>("media_directory", star::Config_Settings::mediadirectory),
    std::pair<std::string, star::Config_Settings>("texture_filtering", star::Config_Settings::texture_filtering),
    std::pair<std::string, star::Config_Settings>("texture_anisotropy", star::Config_Settings::texture_anisotropy),
    std::pair<std::string, star::Config_Settings>("frames_in_flight", star::Config_Settings::frames_in_flight),
    std::make_pair("required_device_feature_shader_float64",
                   star::Config_Settings::required_device_feature_shader_float64),
    std::make_pair("resolution_x", star::Config_Settings::resolution_x),
    std::make_pair("resolution_y", star::Config_Settings::resolution_y),
    std::make_pair("tmp_dir", star::Config_Settings::tmp_directory),
    std::make_pair("scene_file", star::Config_Settings::scene_file),
    std::make_pair("max_image_worker_count", star::Config_Settings::max_image_worker_count)};

void star::ConfigFile::load(const std::filesystem::path &configPath)
{
    auto raw = config::ConfigReader{}(configPath);
    if (!raw.has_value())
    {
        std::ostringstream oss;
        oss << "Provided config file was not found or could not be read: " << configPath.string() << std::endl
            << "A default config file should have been created with the starlight app builder project" << std::endl;

        STAR_THROW(oss.str());
    }

    applyDefaults(raw.value());
}

void star::ConfigFile::load(const std::map<std::string, std::string> &rawValues)
{
    applyDefaults(rawValues);
}

void star::ConfigFile::applyDefaults(std::map<std::string, std::string> values)
{
    for (auto &[jsonKey, configKey] : availableSettings)
    {
        if (auto it = values.find(jsonKey); it != values.end())
        {
            settings[configKey] = it->second;
            values.erase(it);
        }
        else
        {
            switch (configKey)
            {
            case Config_Settings::texture_filtering:
                settings[configKey] = "linear";
                break;
            case Config_Settings::texture_anisotropy:
                settings[configKey] = "0";
                break;
            case Config_Settings::frames_in_flight:
                settings[configKey] = "2";
                break;
            case Config_Settings::tmp_directory: {
                const auto path = file_helpers::GetExecutableDirectory() / "tmp";
                settings[configKey] = path.string();

                if (!std::filesystem::exists(path))
                {
                    core::logging::info("Creating temporary data directory: " + path.string());
                    std::filesystem::create_directories(path);
                }
                break;
            }
            case Config_Settings::scene_file:
                settings[configKey] = "StarScene.json";
                break;
            case Config_Settings::max_image_worker_count:
                settings[configKey] = "8";
                break;
            default:
                STAR_THROW("Setting not found and has no available default: " + jsonKey);
            }
        }
    }
}

std::string star::ConfigFile::getSetting(Config_Settings setting)
{
    auto it = settings.find(setting);
    if (it != settings.end())
    {
        return it->second;
    }

    std::string name;
    switch (setting)
    {
    case (Config_Settings::app_name):
        name = "app_name";
        break;
    case (Config_Settings::frames_in_flight):
        name = "frames_in_flight";
        break;
    case (Config_Settings::mediadirectory):
        name = "media_directory";
        break;
    case (Config_Settings::required_device_feature_shader_float64):
        name = "required_device_feature_shader_float64";
        break;
    case (Config_Settings::resolution_x):
        name = "resolution_x";
        break;
    case (Config_Settings::resolution_y):
        name = "resolution_y";
        break;
    case (Config_Settings::texture_anisotropy):
        name = "texture_anisotropy";
        break;
    case (Config_Settings::texture_filtering):
        name = "texture_filtering";
        break;
    case (Config_Settings::tmp_directory):
        name = "tmp_directory";
        break;
    case (Config_Settings::scene_file):
        name = "scene_file";
        break;
    case (Config_Settings::max_image_worker_count):
        name = "max_image_worker_count";
        break;
    default:
        name = "UNKNOWN";
        break;
    }

    std::ostringstream oss;
    oss << "Setting not found: " << name << std::endl;
    STAR_THROW(oss.str());
}
