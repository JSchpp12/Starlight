#pragma once

#include "graphics/StarBuffers/Buffer.hpp"

namespace star::wrappers::graphics::policies
{
struct GenericBufferCreateAllocatePolicy
{
    using Object = star::StarBuffers::Buffer;

    mutable VmaAllocator allocator;
    std::string allocationName;
    VmaAllocationCreateInfo allocInfo;
    vk::BufferCreateInfo createInfo;
    vk::DeviceSize instanceSize = 0;
    uint32_t instanceCount = 0;
    vk::DeviceSize minOffsetAlignment = 1;

    Object create() const
    {
        return Object(allocator, instanceCount, instanceSize, minOffsetAlignment, allocInfo, createInfo,
                      allocationName);
    }
};
} // namespace star::wrappers::graphics::policies