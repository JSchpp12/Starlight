#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>

namespace star::io
{
/// @brief Describes why stb could not decode a PNG file.
class PngLoadException : public std::runtime_error
{
  public:
    PngLoadException(std::filesystem::path path, std::string reason)
        : std::runtime_error("Failed to load PNG file '" + path.string() + "': " + reason),
          m_path(std::move(path)), m_reason(std::move(reason))
    {
    }

    const std::filesystem::path &path() const noexcept
    {
        return m_path;
    }

    const std::string &reason() const noexcept
    {
        return m_reason;
    }

  private:
    std::filesystem::path m_path;
    std::string m_reason;
};
} // namespace star::io
