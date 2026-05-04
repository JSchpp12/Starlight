#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>

namespace star::config {

/// Stateless reader that loads raw JSON string values into a map.
struct ConfigReader {

    /// Reads and parses the JSON config at \p configPath.
    /// Returns a map of JSON key -> string value.
    /// Returns std::nullopt on failure, after logging the error.
    std::optional<std::map<std::string, std::string>>
    operator()(const std::filesystem::path &configPath) const;
};
} // namespace star::config
