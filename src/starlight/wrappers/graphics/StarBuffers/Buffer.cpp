#include "StarBuffers/Buffer.hpp"

#include <cassert>

/*
 * Initially based off Ive_buffer by Brendan Galea -
 *https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanBuffer.h
 */

namespace star
{
void StarBuffers::Buffer::map(void **mapped) const
{
    assert(this->resources && this->resources->buffer && this->resources->memory &&
           "Called map on buffer before creation");

    vmaMapMemory(this->resources->allocator, this->resources->memory, mapped);
}

void StarBuffers::Buffer::unmap() const
{
    vmaUnmapMemory(this->resources->allocator, this->resources->memory);
}

void StarBuffers::Buffer::writeToBuffer(void *data, void *mapped, const vk::DeviceSize &size,
                                        const vk::DeviceSize &offset)
{
    assert(mapped && "Invalid mapped memory");

    if (size == vk::WholeSize)
    {
        memcpy(mapped, data, this->size);
    }
    else
    {
        char *memOffset = (char *)mapped;
        memOffset += offset;
        memcpy(memOffset, data, size);
    }
}

vk::Result StarBuffers::Buffer::invalidate(const vk::DeviceSize &size, const vk::DeviceSize &offset) const
{
    auto result = vmaInvalidateAllocation(this->resources->allocator, this->resources->memory, offset, size);
    return vk::Result(result);
}

vk::Result StarBuffers::Buffer::flush(const vk::DeviceSize &size, const vk::DeviceSize &offset) const
{
    auto result = vmaFlushAllocation(this->resources->allocator, this->resources->memory, offset, size);
    return vk::Result(result);
}

vk::DescriptorBufferInfo StarBuffers::Buffer::descriptorInfo(const vk::DeviceSize &size, const vk::DeviceSize &offset)
{
    return vk::DescriptorBufferInfo{this->resources->buffer, offset, size};
}

void StarBuffers::Buffer::writeToIndex(void *data, void *mapped, const size_t &index)
{
    writeToBuffer(data, mapped, this->instanceSize, index * m_alignmentSize);
}

vk::Result StarBuffers::Buffer::flushIndex(const size_t &index)
{
    return flush(m_alignmentSize, index * m_alignmentSize);
}

vk::DescriptorBufferInfo StarBuffers::Buffer::descriptorInfoForIndex(const size_t &index)
{
    return descriptorInfo(m_alignmentSize, index * m_alignmentSize);
}

StarBuffers::Buffer::Buffer(std::shared_ptr<Resources> resources, const uint32_t &instanceCount,
                            const vk::DeviceSize &instanceSize, const vk::DeviceSize &alignmentSize,
                            const vk::DeviceSize &offset, const vk::DeviceSize &size,
                            const vk::BufferUsageFlags &usageFlags)
    : resources(std::move(resources)), size(size), offset(offset), instanceSize(instanceSize),
      instanceCount(instanceCount), m_alignmentSize(alignmentSize), usageFlags(usageFlags)
{
}

void StarBuffers::Buffer::cleanupRender(vk::Device &device)
{
    (void)device;

    if (resources)
    {
        resources->cleanupRender();
    }
}
} // namespace star
