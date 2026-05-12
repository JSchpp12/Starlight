#pragma once

#include "enums/Enums.hpp"

#include <filesystem>
#include <map>
#include <string>

namespace star::config {
struct ConfigReader;
} // namespace star::config

namespace star {

class ConfigFile {
public:
    ConfigFile() = default;
    ~ConfigFile() = default;

    /// Load config from disk (for simple use cases without I/O threading)
    static void load(const std::filesystem::path &configPath);

    /// Load config from a pre-read raw map (e.g. from your I/O thread)
    static void load(const std::map<std::string, std::string> &rawValues);

    static std::string getSetting(Config_Settings setting);

    /// Parse a typed config value with a default fallback.
    /// Throws via ConfigFile::getSetting if the key is entirely missing from the loaded config;
    /// returns the default when the key exists but parsing fails.
    static uint32_t getUint32(Config_Settings setting, uint32_t defaultVal);
    static int getInt(Config_Settings setting, int defaultVal);
    static double getDouble(Config_Settings setting, double defaultVal);
    static std::string getString(Config_Settings setting, std::string_view defaultVal);

private:
    /// Applies defaults for any missing keys into `m_settings`
    static void applyDefaults(std::map<std::string, std::string> values);

    static std::map<Config_Settings, std::string> settings;
    static std::map<std::string, Config_Settings> availableSettings;
};
} // namespace star
