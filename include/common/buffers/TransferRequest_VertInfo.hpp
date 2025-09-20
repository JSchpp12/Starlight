#pragma once

#include "TransferRequest_Buffer.hpp"

#include "Vertex.hpp"

namespace star::TransferRequest
{
class VertInfo : public Buffer
{
  public:
    VertInfo(const uint32_t &graphicsQueueIndex, std::vector<Vertex> vertices)
        : vertices(vertices), graphicsQueueIndex(graphicsQueueIndex)
    {
    }

    std::unique_ptr<StarBuffers::Buffer> createStagingBuffer(vk::Device &device, VmaAllocator &allocator) const override;

    std::unique_ptr<StarBuffers::Buffer> createFinal(vk::Device &device, VmaAllocator &allocator,
                                            const std::vector<uint32_t> &transferQueueFamilyIndex) const override;

    void writeDataToStageBuffer(StarBuffers::Buffer &buffer) const override;

  protected:
    const uint32_t graphicsQueueIndex;
    std::vector<Vertex> vertices;
};
} // namespace star::TransferRequest