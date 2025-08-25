#pragma once

#include "StarBuffers/Buffer.hpp"
#include "devices/StarDevice.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>
#include <vector>

namespace star::TransferRequest
{
/// Request object used by secondary thread manager to copy resources to GPU. Templated to allow for different types of
/// creation args such as BufferCreationArgs.
template <typename T> class Memory
{
  public:
    Memory() = default;
    ~Memory() = default;

    virtual std::unique_ptr<StarBuffers::Buffer> createStagingBuffer(vk::Device &device, VmaAllocator &allocator) const = 0;

    virtual std::unique_ptr<T> createFinal(vk::Device &device, VmaAllocator &allocator,
                                           const std::vector<uint32_t> &allTransferQueueFamilyIndices) const = 0;

    virtual void copyFromTransferSRCToDST(StarBuffers::Buffer &srcBuffer, T &dst, vk::CommandBuffer &commandBuffer) const = 0;

    virtual void writeDataToStageBuffer(StarBuffers::Buffer &buffer) const = 0; 

  protected:
};
} // namespace star::TransferRequest
