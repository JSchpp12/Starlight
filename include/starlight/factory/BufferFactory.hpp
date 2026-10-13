#pragma once

#include "StarBuffers/Buffer.hpp"
#include "StarBuffers/Resources.hpp"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include <cstdint>
#include <memory>
#include <string>

namespace star::StarBuffers
{

/// @brief Builds the underlying Vulkan objects for a buffer.
class BufferFactory
{
  public:
    static std::shared_ptr<Resources> CreateResource(core::device::StarDevice &device, VmaAllocator &allocator,
                                                     const VmaAllocationCreateInfo &allocationCreateInfo,
                                                     const vk::BufferCreateInfo &bufferCreateInfo,
                                                     const std::string &allocationName, vk::DeviceSize &offset);

    /// @brief Build a complete Buffer for a newly allocated vulkan buffer.
    /// @param device
    /// @param creators
    /// @param instanceCount
    /// @param instanceSize
    /// @param minOffsetAlignment
    /// @return
    static Buffer CreateBuffer(core::device::StarDevice &device, const Buffer::Creators &creators,
                               uint32_t instanceCount, const vk::DeviceSize &instanceSize,
                               const vk::DeviceSize &minOffsetAlignment);

    /// @brief Calculate the aligned size of a single instance within a buffer.
    /// @param instanceSize
    /// @param minOffsetAlignment
    /// @return
    static vk::DeviceSize GetAlignment(const vk::DeviceSize &instanceSize, const vk::DeviceSize &minOffsetAlignment);

  private:
    static void VerifyNewAllocation(const Buffer::Creators &creators, uint32_t instanceCount,
                                    const vk::DeviceSize &instanceSize);

    static Buffer FinishNewAllocation(const Buffer::Creators &creators, uint32_t instanceCount,
                                      const vk::DeviceSize &instanceSize, const vk::DeviceSize &minOffsetAlignment,
                                      const vk::DeviceSize &offset, std::shared_ptr<Resources> resources);

    static void CreateAllocation(core::device::StarDevice &device, VmaAllocator &allocator,
                                 const VmaAllocationCreateInfo &allocationCreateInfo,
                                 const vk::BufferCreateInfo &bufferCreateInfo, VmaAllocation &allocation,
                                 vk::Buffer &buffer, const std::string &allocationName,
                                 VmaAllocationInfo &allocationInfo);

    static void VerifyBufferCreateInfo(const vk::BufferCreateInfo &createInfo);

    static void LogBufferCreateFailure(const vk::Result &result);
};
} // namespace star::StarBuffers
