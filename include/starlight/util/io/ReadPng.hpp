#pragma once

#include "starlight/structs/ImageFileInfo.hpp"
#include "starlight/structs/PngData.hpp"
#include "starlight/util/io/ImageLoadException.hpp"
#include "starlight/util/io/ImageParseException.hpp"
#include "starlight/util/io/PathResolver.hpp"

#include <expected>
#include <filesystem>

namespace star::io
{
/// @brief Reads header information and pixel data from a PNG image file.
class ReadPng
{
  public:
    /// @param path Path of the PNG file, resolved through @p pathResolver before every read.
    /// @param pathResolver Resolver used to locate the file; defaults to the engine's media search policy.
    explicit ReadPng(std::filesystem::path path, PathResolver pathResolver = PathResolver{});

    /// @brief Reads the file header and returns the image's size, resolution and channel format.
    /// @returns The image info, or the reason the header could not be parsed.
    std::expected<ImageFileInfo, ImageParseException> getInfo() const noexcept;

    /// @brief Loads the file's pixel data as RGBA8.
    /// @returns The pixel data, or the reason stb could not decode the file.
    std::expected<PngData, ImageLoadException> read() const;

  private:
    std::filesystem::path m_path;
    PathResolver m_pathResolver;
};
} // namespace star::io
