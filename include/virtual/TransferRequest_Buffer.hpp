#pragma once

#include "TransferRequest_Memory.hpp"
#include "StarBuffers/Buffer.hpp"

namespace star::TransferRequest
{
class Buffer : public Memory<StarBuffers::Buffer>
{
  public:
    Buffer() = default;
    virtual ~Buffer() = default;

    virtual std::unique_ptr<StarBuffers::Buffer> createStagingBuffer(
        vk::Device &device, VmaAllocator &allocator) const override = 0;

    virtual std::unique_ptr<StarBuffers::Buffer> createFinal(vk::Device &device, VmaAllocator &allocator,
                                                    const std::vector<uint32_t> &transferQueueFamilyIndex) const override = 0;

    virtual void copyFromTransferSRCToDST(StarBuffers::Buffer &srcBuffer, StarBuffers::Buffer &dst,
                                          vk::CommandBuffer &commandBuffer) const override;

    virtual void writeDataToStageBuffer(StarBuffers::Buffer &buffer) const override = 0;

  protected:
    static void DefaultCopy(StarBuffers::Buffer &srcBuffer, StarBuffers::Buffer &dstBuffer, vk::CommandBuffer &commandBuffer);

  private:
};
} // namespace star::TransferRequest