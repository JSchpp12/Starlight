#pragma once

#include "TransferRequest_Buffer.hpp"

#include <vector>

namespace star::TransferRequest
{
class IndicesInfo : public Buffer
{
  public:
    IndicesInfo(const std::vector<uint32_t> &indices, const uint32_t &graphicsQueueFamilyIndex)
        : indices(indices), graphicsQueueFamilyIndex(graphicsQueueFamilyIndex)
    {
    }

    std::unique_ptr<StarBuffers::Buffer> createStagingBuffer(vk::Device &device, VmaAllocator &allocator) const override;

    std::unique_ptr<StarBuffers::Buffer> createFinal(vk::Device &device, VmaAllocator &allocator,
                                            const std::vector<uint32_t> &transferQueueFamilyIndex) const override;

    void writeDataToStageBuffer(StarBuffers::Buffer &buffer) const override;

  protected:
    const std::vector<uint32_t> indices;
    const uint32_t graphicsQueueFamilyIndex;
};
} // namespace star::TransferRequest