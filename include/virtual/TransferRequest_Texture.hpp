#pragma once

#include "StarTextures/Texture.hpp"
#include "TransferRequest_Memory.hpp"


#include <vulkan/vulkan.hpp>

namespace star::TransferRequest
{
class Texture : private Memory<star::StarTextures::Texture>
{
  public:
    Texture() = default;
    virtual ~Texture() = default;

    virtual std::unique_ptr<StarBuffer> createStagingBuffer(
        vk::Device &device, VmaAllocator &allocator) const override = 0;

    virtual std::unique_ptr<star::StarTextures::Texture> createFinal(
        vk::Device &device, VmaAllocator &allocator,
        const std::vector<uint32_t> &transferQueueFamilyIndex) const override = 0;

    virtual void copyFromTransferSRCToDST(StarBuffer &srcBuffer, star::StarTextures::Texture &dst,
                                          vk::CommandBuffer &commandBuffer) const override = 0;

    virtual void writeDataToStageBuffer(star::StarBuffer &stagingBuffer) const override = 0;

  protected:
};
} // namespace star::TransferRequest