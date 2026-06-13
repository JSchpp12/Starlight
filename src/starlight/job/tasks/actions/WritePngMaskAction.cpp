#include "job/tasks/actions/WritePngMaskAction.hpp"

#include "job/tasks/actions/WriteImageActions.hpp"

#include "logging/LoggingFactory.hpp"

#include "starlight/core/Exceptions.hpp"
#include <star_common/helper/CastHelpers.hpp>

#include <algorithm>
#include <filesystem>
#include <stb_image_write.h>
#include <vector>

namespace star::job::tasks::actions
{

bool IsPngMaskFormat(vk::Format fmt)
{
    return fmt == vk::Format::eR8Uint;
}

void WritePngMaskAction::operator()()
{
    ValidateExtension(path, ".png");

    const uint32_t width = imageExtent.width;
    const uint32_t height = imageExtent.height;

    if (!IsPngMaskFormat(imageFormat))
    {
        STAR_THROW("Unsupported image format for PNG mask writing. Only eR8Uint is supported.");
    }

    const uint8_t *srcData = nullptr;
    bool needsUnmap = false;
    const StarBuffers::Buffer *bufferToUnmap = nullptr;
    std::vector<uint8_t> scaledData;

    if (auto *bufSrc = std::get_if<VulkanBufferSource>(&dataSource))
    {
        void *mapped = nullptr;
        bufSrc->buffer.map(&mapped);
        if (!mapped)
        {
            STAR_THROW("Failed to map buffer for PNG mask image write");
        }
        bufSrc->buffer.invalidate();
        const uint8_t *mappedData = static_cast<const uint8_t *>(mapped);

        const size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
        scaledData.resize(pixelCount);
        std::transform(mappedData, mappedData + pixelCount, scaledData.begin(),
                       [](uint8_t val) { return static_cast<uint8_t>(val * 255); });
        srcData = scaledData.data();

        bufSrc->buffer.unmap();
    }
    else if (auto *rawSrc = std::get_if<RawUint8Source>(&dataSource))
    {
        srcData = rawSrc->data;
    }
    else
    {
        STAR_THROW("PNG mask writing requires VulkanBufferSource or RawUint8Source data source");
    }

    int w = 0;
    star::common::casts::SafeCast(width, w);
    int h = 0;
    star::common::casts::SafeCast(height, h);
    constexpr int comp = 1;
    const int rowStride = w * comp;

    int ok = stbi_write_png(path.c_str(), w, h, comp, srcData, rowStride);

    if (ok == 0)
    {
        STAR_THROW("Failed to write PNG mask image to disk");
    }
}

} // namespace star::job::tasks::actions