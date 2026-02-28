#pragma once

#include <vulkan/vulkan.hpp>

namespace star::StarTextures
{
class FormatInfo
{
  public:
    bool blockCompressed;
    uint32_t bytesPerPixel;   // for uncompressed
    vk::Extent3D blockExtent; // for compressed (block width/height/depth)
    uint32_t blockBytes;      // for compressed

    static FormatInfo Create(const vk::Format &fmt); 

    FormatInfo(bool blockCompressed, uint32_t bytesPerPixel, vk::Extent3D blockExtent, uint32_t blockBytes)
        : blockCompressed(std::move(blockCompressed)), bytesPerPixel(std::move(bytesPerPixel)),
          blockExtent(std::move(blockExtent)), blockBytes(std::move(blockBytes))
    {
    }
};
} // namespace star::StarTextures