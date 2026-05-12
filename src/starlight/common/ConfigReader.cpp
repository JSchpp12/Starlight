#include "common/ConfigReader.hpp"

#include "core/Exceptions.hpp"
#include "logging/LoggingFactory.hpp"

#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>

namespace star::config
{
using json = nlohmann::json;

std::optional<std::map<std::string, std::string>> ConfigReader::operator()(
    const std::filesystem::path &configPath) const
{
    // File existence check
    if (!std::filesystem::exists(configPath))
    {
        core::logging::error("Config file not found: ", configPath.string());
        return std::nullopt;
    }

    json configJson;
    try
    {
        std::ifstream stream(configPath);
        stream >> configJson;
        if (stream.fail() || !configJson.is_object())
        {
            core::logging::error("Failed to parse config file: ", configPath.string());
            return std::nullopt;
        }
    }
    catch (const std::exception &e)
    {
        core::logging::error("Error reading config file: ", e.what());
        return std::nullopt;
    }

    auto values = std::map<std::string, std::string>{};
    for (auto it = configJson.begin(); it != configJson.end(); ++it)
    {
        values[it.key()] = it.value().get<std::string>();
    }
    return values;
}
} // namespace star::config
