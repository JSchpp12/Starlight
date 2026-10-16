#pragma once

#include "StarBuffers/Buffer.hpp"
#include "StarBuffers/Resources.hpp"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include <cstdint>
#include <memory>
#include <string>

namespace star::StarBuffers::BufferFactory
{
std::shared_ptr<Resources> CreateResource(core::device::StarDevice &device, VmaAllocator &allocator,
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
Buffer CreateBuffer(core::device::StarDevice &device, const Buffer::Creators &creators, uint32_t instanceCount,
                    const vk::DeviceSize &instanceSize, const vk::DeviceSize &minOffsetAlignment);

/// @brief Calculate the aligned size of a single instance within a buffer.
/// @param instanceSize
/// @param minOffsetAlignment
/// @return
vk::DeviceSize GetAlignment(const vk::DeviceSize &instanceSize, const vk::DeviceSize &minOffsetAlignment);
} // namespace star::StarBuffers::BufferFactory
