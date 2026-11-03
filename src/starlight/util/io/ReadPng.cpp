#include "starlight/util/io/ReadPng.hpp"

#include "starlight/core/Exceptions.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <string>
#include <utility>

namespace star::io
{
namespace
{
ImageChannelFormat ToChannelFormat(int channels)
{
    switch (channels)
    {
    case 1:
        return ImageChannelFormat::Grayscale;
    case 2:
        return ImageChannelFormat::GrayscaleAlpha;
    case 3:
        return ImageChannelFormat::Rgb;
    case 4:
        return ImageChannelFormat::Rgba;
    default:
        STAR_THROW("Unsupported channel count in image file: " + std::to_string(channels));
    }
}
} // namespace

ReadPng::ReadPng(std::filesystem::path path) : m_path(std::move(path))
{
}

ImageFileInfo ReadPng::getInfo() const
{
    const std::string pathString = m_path.string();

    int width = 0, height = 0, channels = 0;
    if (!stbi_info(pathString.c_str(), &width, &height, &channels))
    {
        STAR_THROW("Failed to read image file info: " + pathString);
    }

    if (width <= 0 || height <= 0)
    {
        STAR_THROW("Image file has an invalid resolution: " + pathString);
    }

    const auto format = ToChannelFormat(channels);
    const auto texelSize = GetSizeInMemory(format);
    return ImageFileInfo{
        .size = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * texelSize,
        .resolution = glm::vec3(static_cast<float>(width), static_cast<float>(height), 1.0f),
        .format = format,
    };
}

std::expected<PngData, PngLoadException> ReadPng::read() const
{
    const std::string pathString = m_path.string();

    int width = 0, height = 0, channels = 0;
    unsigned char *pixels = stbi_load(pathString.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels)
    {
        const char *failureReason = stbi_failure_reason();
        return std::unexpected(PngLoadException(m_path, failureReason ? failureReason : "unknown error"));
    }

    // stbi_load with STBI_rgb_alpha always expands the file to four channels.
    const auto size = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) *
                      GetSizeInMemory(ImageChannelFormat::Rgba);

    return PngData{pixels, size};
}
} // namespace star::io
