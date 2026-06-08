#pragma once

#include "TransferRequest_Buffer.hpp"
#include "starlight/structs/Color.hpp"

#include <glm/glm.hpp>

namespace star::TransferRequest
{
/// <summary>
/// Container for transfering color information to a uniform buffer for instance based rendering. Particularly useful in
/// the InstanceColor material.
/// </summary>
class InstanceColorInfo : public star::TransferRequest::Buffer
{
  public:
    InstanceColorInfo(std::vector<star::Color> colors, uint32_t graphicsQueueFamilyIndex);

    std::unique_ptr<StarBuffers::Buffer> createStagingBuffer(vk::Device &device,
                                                             VmaAllocator &allocator) const override;

    std::unique_ptr<StarBuffers::Buffer> createFinal(
        vk::Device &device, VmaAllocator &allocator,
        const std::vector<uint32_t> &transferQueueFamilyIndex) const override;

    void writeDataToStageBuffer(StarBuffers::Buffer &buffer) const override;

  protected:
    std::vector<star::Color> m_colors;
    uint32_t m_graphicsQueueFamilyIndex;
};
} // namespace star::TransferRequest