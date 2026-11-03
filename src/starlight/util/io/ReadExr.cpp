#include "starlight/util/io/ReadExr.hpp"

#include <tinyexr.h>

#include <cstddef>
#include <optional>
#include <string>
#include <utility>

namespace star::io
{
namespace
{
std::optional<ImageChannelFormat> ToChannelFormat(int channels) noexcept
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
} // namespace

ReadExr::ReadExr(std::filesystem::path path) : m_path(std::move(path))
{
}

std::expected<ImageFileInfo, ImageParseException> ReadExr::getInfo() const noexcept
{
    const std::string pathString = m_path.string();

    EXRVersion version{};
    if (ParseEXRVersionFromFile(&version, pathString.c_str()) != TINYEXR_SUCCESS)
    {
        return std::unexpected(ImageParseException(m_path, "failed to read EXR version info"));
    }

    EXRHeader header{};
    InitEXRHeader(&header);

    const char *error = nullptr;
    if (ParseEXRHeaderFromFile(&header, &version, pathString.c_str(), &error) != TINYEXR_SUCCESS)
    {
        const std::string reason = error ? error : "unknown error";
        FreeEXRErrorMessage(error);
        return std::unexpected(ImageParseException(m_path, reason));
    }

    const int width = header.data_window.max_x - header.data_window.min_x + 1;
    const int height = header.data_window.max_y - header.data_window.min_y + 1;
    if (width <= 0 || height <= 0)
    {
        FreeEXRHeader(&header);
        return std::unexpected(ImageParseException(m_path, "invalid resolution"));
    }

    // HALF channels occupy two bytes per sample; FLOAT and UINT occupy four.
    std::size_t bytesPerPixel = 0;
    for (int i = 0; i < header.num_channels; ++i)
    {
        bytesPerPixel += header.pixel_types[i] == TINYEXR_PIXELTYPE_HALF ? 2u : 4u;
    }

    const int channelCount = header.num_channels;
    FreeEXRHeader(&header);

    const auto format = ToChannelFormat(channelCount);
    if (!format)
    {
        return std::unexpected(
            ImageParseException(m_path, "unsupported channel count: " + std::to_string(channelCount)));
    }

    return ImageFileInfo{
        .size = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * bytesPerPixel,
        .resolution = glm::vec3(static_cast<float>(width), static_cast<float>(height), 1.0f),
        .format = *format,
    };
}

std::expected<ExrData, ImageLoadException> ReadExr::read() const
{
    const std::string pathString = m_path.string();

    float *pixels = nullptr;
    int width = 0;
    int height = 0;
    const char *error = nullptr;

    if (LoadEXR(&pixels, &width, &height, pathString.c_str(), &error) != TINYEXR_SUCCESS)
    {
        const std::string reason = error ? error : "unknown error";
        FreeEXRErrorMessage(error);
        return std::unexpected(ImageLoadException(m_path, reason));
    }

    // LoadEXR always writes four (RGBA) float channels per pixel.
    const auto size = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u;
    return ExrData{pixels, size};
}
} // namespace star::io
