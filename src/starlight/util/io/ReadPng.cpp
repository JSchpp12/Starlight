#include "starlight/util/io/ReadPng.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <optional>
#include <string>
#include <utility>

namespace star::io
{

static std::optional<ImageChannelFormat> ToChannelFormat(int channels) noexcept
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
        return std::nullopt;
    }
}

ReadPng::ReadPng(std::filesystem::path path, PathResolver pathResolver)
    : m_path(std::move(path)), m_pathResolver(std::move(pathResolver))
{
}

std::expected<ImageFileInfo, ImageParseException> ReadPng::getInfo() const noexcept
{
    const auto resolved = m_pathResolver.resolve(m_path, PathKind::file);
    if (!resolved)
        return std::unexpected(ImageParseException(m_path, resolved.error().toString()));

    const std::string pathString = resolved->string();

    int width = 0, height = 0, channels = 0;
    if (!stbi_info(pathString.c_str(), &width, &height, &channels))
    {
        const char *failureReason = stbi_failure_reason();
        return std::unexpected(ImageParseException(*resolved, failureReason ? failureReason : "unknown error"));
    }

    if (width <= 0 || height <= 0)
    {
        return std::unexpected(ImageParseException(*resolved, "invalid resolution"));
    }

    const auto format = ToChannelFormat(channels);
    if (!format)
    {
        return std::unexpected(
            ImageParseException(*resolved, "unsupported channel count: " + std::to_string(channels)));
    }

    const auto texelSize = GetSizeInMemory(*format);
    return ImageFileInfo{
        .size = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * texelSize,
        .resolution = glm::vec3(static_cast<float>(width), static_cast<float>(height), 1.0f),
        .format = *format,
    };
}

std::expected<PngData, ImageLoadException> ReadPng::read() const
{
    const auto resolved = m_pathResolver.resolve(m_path, PathKind::file);
    if (!resolved)
        return std::unexpected(ImageLoadException(m_path, resolved.error().toString()));

    const std::string pathString = resolved->string();

    int width = 0, height = 0, channels = 0;
    unsigned char *pixels = stbi_load(pathString.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels)
    {
        const char *failureReason = stbi_failure_reason();
        return std::unexpected(ImageLoadException(*resolved, failureReason ? failureReason : "unknown error"));
    }

    // stbi_load with STBI_rgb_alpha always expands the file to four channels.
    const auto size =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * GetSizeInMemory(ImageChannelFormat::Rgba);

    return PngData{pixels, size};
}
} // namespace star::io
