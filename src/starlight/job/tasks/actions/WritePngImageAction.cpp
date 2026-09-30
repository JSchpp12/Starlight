#include "job/tasks/actions/WritePngImageAction.hpp"

#include "job/tasks/actions/WriteImageActions.hpp"

#include "logging/LoggingFactory.hpp"

#include "starlight/core/Exceptions.hpp"
#include <star_common/helper/CastHelpers.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <stb_image_write.h>
#include <vector>

namespace star::job::tasks::actions
{

bool IsPngFormat(vk::Format fmt)
{
    return fmt == vk::Format::eR8G8B8A8Unorm || fmt == vk::Format::eR8G8B8A8Srgb || fmt == vk::Format::eB8G8R8A8Unorm ||
           fmt == vk::Format::eB8G8R8A8Srgb;
}

void WritePngImageAction::operator()()
{
    ValidateExtension(path, ".png");

    const uint32_t width = imageExtent.width;
    const uint32_t height = imageExtent.height;

    if (!IsPngFormat(imageFormat))
    {
        STAR_THROW("Unsupported image format for PNG writing.");
    }

    const void *providedDataSource = nullptr;
    bool needsUnmap = false;
    const StarBuffers::Buffer *bufferToUnmap = nullptr;

    if (auto *bufSrc = std::get_if<VulkanBufferSource>(&dataSource))
    {
        void *mapped = nullptr;
        bufSrc->buffer.map(&mapped);
        if (!mapped)
        {
            STAR_THROW("Failed to map buffer for PNG image write");
        }
        bufSrc->buffer.invalidate();
        providedDataSource = mapped;
        needsUnmap = true;
        bufferToUnmap = &bufSrc->buffer;
    }
    else if (auto *rawSrc = std::get_if<RawUint8Source>(&dataSource))
    {
        providedDataSource = rawSrc->data;
    }
    else
    {
        STAR_THROW("PNG image writing requires VulkanBufferSource or RawUint8Source data source");
    }

    int comp = 4;
    int w = 0;
    star::common::casts::SafeCast(width, w);
    int h = 0;
    star::common::casts::SafeCast(height, h);
    const int rowStride = w * comp;

    // stbi_write_png always interprets the buffer as R,G,B,A. Vulkan
    // eB8G8R8A8* sources have the R and B bytes swapped, so normalize here.
    const bool isBgra = imageFormat == vk::Format::eB8G8R8A8Unorm ||
                        imageFormat == vk::Format::eB8G8R8A8Srgb;

    int stbiWriteResult{0}; 
    if (isBgra)
    {
        const size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
        const size_t byteCount = pixelCount * 4u;
        std::vector<uint8_t> swizzled = std::vector<uint8_t>(byteCount);
        const auto *src = static_cast<const uint8_t *>(providedDataSource);
        for (size_t i = 0; i < byteCount; i += 4)
        {
            swizzled[i + 0] = src[i + 2]; // R <- B
            swizzled[i + 1] = src[i + 1]; // G
            swizzled[i + 2] = src[i + 0]; // B <- R
            swizzled[i + 3] = src[i + 3]; // A
        }
        stbiWriteResult = stbi_write_png(path.c_str(), w, h, comp, swizzled.data(), rowStride);
    }else{
        stbiWriteResult = stbi_write_png(path.c_str(), w, h, comp, providedDataSource, rowStride); 
    }

    if (needsUnmap)
    {
        bufferToUnmap->unmap();
    }

    if (stbiWriteResult == 0)
    {
        STAR_THROW("Failed to write PNG image to disk");
    }
}

} // namespace star::job::tasks::actions