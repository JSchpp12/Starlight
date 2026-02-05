#include "StarBuffers/Buffer.hpp"

#include "logging/LoggingFactory.hpp"

#include <sstream>
#include <star_common/helper/CastHelpers.hpp>

/*
 * Initially based off Ive_buffer by Brendan Galea -
 *https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanBuffer.h
 */

namespace star
{
vk::DeviceSize StarBuffers::Buffer::GetAlignment(const vk::DeviceSize &instanceSize,
                                                 const vk::DeviceSize &minOffsetAlignment)
{
    if (minOffsetAlignment > 0)
    {
        return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
    }
    return instanceSize;
}

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

StarBuffers::Buffer::Buffer(VmaAllocator &allocator, const uint32_t &requestedInstanceCount,
                            const vk::DeviceSize &requestedInstanceSize, const vk::DeviceSize &minOffsetAlignment,
                            const VmaAllocationCreateInfo &allocCreateInfo,
                            const vk::BufferCreateInfo &bufferCreateInfo, const std::string &allocName)
    : instanceSize(requestedInstanceSize), m_alignmentSize(GetAlignment(instanceSize, minOffsetAlignment)),
      usageFlags(bufferCreateInfo.usage)
{
    assert(bufferCreateInfo.size != 0 && requestedInstanceCount != 0 && requestedInstanceSize != 0);

    if (!star::common::helper::SafeCast<vk::DeviceSize, uint32_t>(requestedInstanceCount, this->instanceCount))
        throw std::runtime_error("Failed to cast instance count");

    this->resources = CreateBuffer(allocator, allocCreateInfo, bufferCreateInfo, this->size, this->offset, allocName);
}

std::string StarBuffers::Buffer::AllocationName(const std::string &allocationName)
{
    return allocationName + "_BUFFER";
}
std::shared_ptr<star::StarBuffers::Resources> StarBuffers::Buffer::CreateBuffer(
    VmaAllocator &allocator, const VmaAllocationCreateInfo &allocCreateInfo, const vk::BufferCreateInfo &bufferInfo,
    vk::DeviceSize &resultingBufferSize, vk::DeviceSize &resultingBufferOffset, const std::string &allocationName)
{
    VmaAllocationInfo allocationInfo;
    vk::Buffer buffer;
    VmaAllocation memory;

    auto result = (vk::Result)vmaCreateBuffer(allocator, (VkBufferCreateInfo *)&bufferInfo, &allocCreateInfo,
                                              (VkBuffer *)&buffer, &memory, &allocationInfo);

    if (result != vk::Result::eSuccess)
    {
        LogAllocationFailure(result);

        throw std::runtime_error("Failed to allocate buffer memory");
    }

    auto fullAllocationName = AllocationName(allocationName);
    vmaSetAllocationName(allocator, memory, fullAllocationName.c_str());

    resultingBufferSize = bufferInfo.size;
    resultingBufferOffset = allocationInfo.offset;

    return std::make_shared<StarBuffers::Resources>(allocator, memory, buffer);
}

void StarBuffers::Buffer::cleanupRender(vk::Device &device)
{
    (void)device;

    if (resources)
    {
        resources->cleanupRender();
    }
}

StarBuffers::Buffer::Builder::Builder(VmaAllocator &allocator) : allocator(allocator)
{
}

StarBuffers::Buffer::Builder &StarBuffers::Buffer::Builder::setAllocationCreateInfo(
    const VmaAllocationCreateInfo &nAllocInfo, const vk::BufferCreateInfo &nBufferInfo, const std::string &nAllocName)
{
    this->allocInfo = nAllocInfo;
    this->allocName = nAllocName;
    this->buffInfo = nBufferInfo;

    return *this;
}

StarBuffers::Buffer::Builder &StarBuffers::Buffer::Builder::setInstanceCount(const uint32_t &nInstanceCount)
{
    this->instanceCount = nInstanceCount;
    return *this;
}

StarBuffers::Buffer::Builder &StarBuffers::Buffer::Builder::setInstanceSize(const vk::DeviceSize &nInstanceSize)
{
    this->instanceSize = nInstanceSize;
    return *this;
}

StarBuffers::Buffer::Builder &StarBuffers::Buffer::Builder::setMinOffsetAlignment(
    const vk::DeviceSize &nMinOffsetAlignment)
{
    this->minOffsetAlignment = nMinOffsetAlignment;
    return *this;
}

std::unique_ptr<StarBuffers::Buffer> StarBuffers::Buffer::Builder::buildUnique()
{
    return std::make_unique<StarBuffers::Buffer>(build());
}

StarBuffers::Buffer StarBuffers::Buffer::Builder::build()
{
    assert(this->instanceCount != 0 && this->instanceSize != 0 && "Instance info must be provided");

    return StarBuffers::Buffer(this->allocator, this->instanceCount, this->instanceSize, this->minOffsetAlignment,
                               this->allocInfo, this->buffInfo, this->allocName);
}

void StarBuffers::Buffer::LogAllocationFailure(const vk::Result &allocationResult)
{
    std::ostringstream oss;
    oss << "Error occurred during buffer allocation: ";

    if (allocationResult == vk::Result::eErrorOutOfDeviceMemory)
    {
        oss << "Out of device memory";
    }
    else
    {
        oss << "Unknown allocation error";
    }

    core::logging::log(boost::log::trivial::fatal, oss.str());
}
} // namespace star