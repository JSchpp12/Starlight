#pragma once

#include "starlight/structs/ImageFileInfo.hpp"
#include "starlight/structs/PngData.hpp"
#include "starlight/util/io/PngLoadException.hpp"

#include <expected>
#include <filesystem>

namespace star::io
{
/// @brief Reads header information and pixel data from a PNG image file.
class ReadPng
{
  public:
    explicit ReadPng(std::filesystem::path path);

    /// @brief Reads the file header and returns the image's size, resolution and channel format.
    /// @throws star::core::RuntimeError if the file cannot be decoded.
    ImageFileInfo getInfo() const;

    /// @brief Loads the file's pixel data as RGBA8.
    /// @returns The pixel data, or the reason stb could not decode the file.
    std::expected<PngData, PngLoadException> read() const;

  private:
    std::filesystem::path m_path;
};
} // namespace star::io
