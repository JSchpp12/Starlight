#pragma once

#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

namespace star::io
{
/// What kind of filesystem object is being requested.
enum class PathKind
{
    file,
    directory
};

/// @brief Everything a caller needs to understand a failed path resolution.
struct IOError
{
    std::filesystem::path requested;              // candidate as given
    PathKind kind{};                              // what was requested
    std::vector<std::filesystem::path> attempted; // every probe, in order
    std::string detail;                           // first underlying fs error message, if any

    std::string toString() const
    {
        std::ostringstream oss;
        oss << "requested " << (kind == PathKind::file ? "file" : "directory") << " '" << requested.string()
            << "' was not found. Attempted locations (in order):";
        for (const auto &attempt : attempted)
        {
            oss << "\n  - " << attempt.string();
        }
        if (!detail.empty())
        {
            oss << "\nFirst underlying filesystem error: " << detail;
        }
        return oss.str();
    }
};
} // namespace star::io
