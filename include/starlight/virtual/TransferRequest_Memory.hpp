#pragma once

#include "StarBuffers/Buffer.hpp"
#include "device/StarDevice.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>
#include <string>
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

    virtual void prep() {};

    /// @brief Publish a terminal failure to any consumer-side promise this request owns.
    /// @details Called by the transfer worker on its outermost failure boundary so consumers receive a blank resource
    /// plus a reason instead of a broken promise.
    virtual void publishFailure(const std::string &message)
    {
        (void)message;
    }

    virtual std::unique_ptr<StarBuffers::Buffer> createStagingBuffer(core::device::StarDevice &device) const = 0;

    virtual std::unique_ptr<T> createFinal(core::device::StarDevice &device,
                                           const std::vector<uint32_t> &allTransferQueueFamilyIndices) const = 0;

    virtual void copyFromTransferSRCToDST(StarBuffers::Buffer &srcBuffer, T &dst,
                                          vk::CommandBuffer &commandBuffer) const = 0;

    virtual void writeDataToStageBuffer(StarBuffers::Buffer &buffer) const = 0;

  protected:
};
} // namespace star::TransferRequest
