#pragma once

#include "TransferRequest_Buffer.hpp"

#include <vector>

namespace star::TransferRequest
{
class IndicesInfo : public Buffer
{
  public:
    IndicesInfo(const uint32_t &graphicsQueueFamilyIndex, std::vector<uint32_t> indices)
        : graphicsQueueFamilyIndex(graphicsQueueFamilyIndex), indices(indices)
    {
    }

    std::unique_ptr<StarBuffers::Buffer> createStagingBuffer(vk::Device &device,
                                                             VmaAllocator &allocator) const override;

    std::unique_ptr<StarBuffers::Buffer> createFinal(
        vk::Device &device, VmaAllocator &allocator,
        const std::vector<uint32_t> &transferQueueFamilyIndex) const override;

    void writeDataToStageBuffer(StarBuffers::Buffer &buffer) const override;

  protected:
    const uint32_t graphicsQueueFamilyIndex;
    const std::vector<uint32_t> indices;
};
} // namespace star::TransferRequest