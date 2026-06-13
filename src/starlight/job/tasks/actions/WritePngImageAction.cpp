#include "job/tasks/actions/WritePngImageAction.hpp"

#include "job/tasks/actions/WriteImageActions.hpp"

#include "logging/LoggingFactory.hpp"

#include "starlight/core/Exceptions.hpp"
#include <star_common/helper/CastHelpers.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <algorithm>
#include <filesystem>
#include <stb_image_write.h>

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

    const void *data = nullptr;
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
        data = mapped;
        needsUnmap = true;
        bufferToUnmap = &bufSrc->buffer;
    }
    else if (auto *rawSrc = std::get_if<RawUint8Source>(&dataSource))
    {
        data = rawSrc->data;
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

    int ok = stbi_write_png(path.c_str(), w, h, comp, data, rowStride);

    if (needsUnmap)
    {
        bufferToUnmap->unmap();
    }

    if (ok == 0)
    {
        STAR_THROW("Failed to write PNG image to disk");
    }
}

} // namespace star::job::tasks::actions