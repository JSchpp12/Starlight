#pragma once

#include <glm/glm.hpp>

#include <cstdint>

namespace star
{
/// @brief Channel layout of the pixel data stored in an image file.
enum class ImageChannelFormat : std::uint8_t
{
    Grayscale,
    GrayscaleAlpha,
    R,
    Rg,
    Rgb,
    Rgba
};

/// @brief Header-level description of an image file, without its pixel data.
struct ImageFileInfo
{
    /// Byte size of the stored pixel data, assuming one byte per channel.
    std::size_t size = 0;
    /// Width, height and depth of the image in pixels.
    glm::vec3 resolution{0.0f, 0.0f, 0.0f};
    /// Channel layout of the stored pixel data.
    ImageChannelFormat format = ImageChannelFormat::Rgba;
};

/// @brief Number of bytes a single texel occupies in memory for the given channel format.
/// PNG channels are eight bits wide, so one byte per channel.
constexpr std::size_t GetSizeInMemory(ImageChannelFormat format) noexcept
{
    switch (format)
    {
    case ImageChannelFormat::Grayscale:
    case ImageChannelFormat::R:
        return 1;
    case ImageChannelFormat::GrayscaleAlpha:
    case ImageChannelFormat::Rg:
        return 2;
    case ImageChannelFormat::Rgb:
        return 3;
    case ImageChannelFormat::Rgba:
        return 4;
    }

    return 0;
}
} // namespace star
