#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>

namespace star::io
{
/// @brief Describes why an image reader could not parse a file's header.
class ImageParseException : public std::runtime_error
{
  public:
    ImageParseException(std::filesystem::path path, std::string reason)
        : std::runtime_error("Failed to parse image file '" + path.string() + "': " + reason), m_path(std::move(path)),
          m_reason(std::move(reason))
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
