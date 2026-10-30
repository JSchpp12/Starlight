#pragma once

#include "graphics/StarBuffers/Buffer.hpp"
#include "starlight/factory/BufferFactory.hpp"

namespace star::wrappers::graphics::policies
{
struct GenericBufferCreateAllocatePolicy
{
    using Object = star::StarBuffers::Buffer;

    mutable VmaAllocator allocator;
    star::core::device::StarDevice *device = nullptr;
    std::string allocationName;
    VmaAllocationCreateInfo allocInfo;
    vk::BufferCreateInfo createInfo;
    vk::DeviceSize instanceSize = 0;
    uint32_t instanceCount = 0;
    vk::DeviceSize minOffsetAlignment = 1;

    Object operator()() const
    {
        vk::DeviceSize offset = 0;
        const auto resources = star::StarBuffers::BufferFactory::CreateResource(*device, allocator, allocInfo,
                                                                                createInfo, allocationName, offset);
        const vk::DeviceSize alignmentSize =
            star::StarBuffers::BufferFactory::GetAlignment(instanceSize, minOffsetAlignment);

        return Object(resources, instanceCount, instanceSize, alignmentSize, offset, createInfo.size, createInfo.usage);
    }
};
} // namespace star::wrappers::graphics::policies
