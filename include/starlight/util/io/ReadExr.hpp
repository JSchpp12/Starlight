#pragma once

#include "starlight/structs/ExrData.hpp"
#include "starlight/structs/ImageFileInfo.hpp"
#include "starlight/util/io/ImageLoadException.hpp"
#include "starlight/util/io/ImageParseException.hpp"

#include <expected>
#include <filesystem>

namespace star::io
{
/// @brief Reads header information and pixel data from an OpenEXR image file.
class ReadExr
{
  public:
    explicit ReadExr(std::filesystem::path path);

    /// @brief Reads the file header and returns the image's size, resolution and channel format.
    /// @returns The image info, or the reason the header could not be parsed.
    std::expected<ImageFileInfo, ImageParseException> getInfo() const noexcept;

    /// @brief Loads the file's pixel data as RGBA floats.
    /// @returns The pixel data, or the reason tinyexr could not decode the file.
    std::expected<ExrData, ImageLoadException> read() const;

  private:
    std::filesystem::path m_path;
};
} // namespace star::io
