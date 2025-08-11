#include "StarBuffers/Buffer.hpp"

/*
 * Initially based off Ive_buffer by Brendan Galea -
 *https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanBuffer.h
 */

namespace star
{
vk::DeviceSize StarBuffers::Buffer::GetAlignment(vk::DeviceSize instanceSize, vk::DeviceSize minOffsetAlignment)
{
    if (minOffsetAlignment > 0)
    {
        return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
    }
    return instanceSize;
}

StarBuffers::Buffer::Buffer(VmaAllocator &allocator, vk::DeviceSize instanceSize, uint32_t instanceCount,
                            const VmaAllocationCreateFlags &creationFlags, const VmaMemoryUsage &memoryUsageFlags,
                            const vk::BufferUsageFlags &useFlags, const vk::SharingMode &sharingMode,
                            const std::string &allocationName, vk::DeviceSize minOffsetAlignment)
    : usageFlags{useFlags}, instanceSize{instanceSize}, instanceCount{instanceCount}
{
    this->alignmentSize = GetAlignment(this->instanceSize, minOffsetAlignment);
    this->bufferSize = this->alignmentSize * instanceCount;

    if (bufferSize == 0)
        throw std::runtime_error("Unable to create a buffer of size 0");

    VmaAllocationInfo allocationInfo{};
    CreateBuffer(allocator, this->bufferSize, useFlags, memoryUsageFlags, creationFlags, allocationInfo,
                 allocationName);

    this->allocationInfo = std::make_optional<VmaAllocationInfo>(allocationInfo);
}

StarBuffers::Buffer::Buffer(VmaAllocator &allocator, const StarBuffers::Buffer::BufferCreationArgs &creationArgs)
    : usageFlags(creationArgs.creationFlags), instanceSize(creationArgs.instanceSize),
      instanceCount(creationArgs.instanceCount)
{
    this->alignmentSize = GetAlignment(this->instanceSize, creationArgs.minOffsetAlignment);
    this->bufferSize = this->alignmentSize * this->instanceCount;

    if (bufferSize == 0)
        throw std::runtime_error("Unable to create a buffer of size 0");

    VmaAllocationInfo allocationInfo{};
    this->resources = CreateBuffer(allocator, this->bufferSize, creationArgs.useFlags, creationArgs.memoryUsageFlags,
                                   creationArgs.creationFlags, allocationInfo, creationArgs.allocationName);

    this->allocationInfo = std::make_optional<VmaAllocationInfo>(allocationInfo);
}

StarBuffers::Buffer::~Buffer()
{
    if (mapped)
        vmaUnmapMemory(this->resources->allocator, this->resources->memory);
}

void StarBuffers::Buffer::map(vk::DeviceSize size, vk::DeviceSize offset)
{
    assert(this->resources->buffer && this->resources->memory && "Called map on buffer before creation");

    vmaMapMemory(this->resources->allocator, this->resources->memory, &this->mapped);
}

void StarBuffers::Buffer::unmap()
{
    if (mapped)
    {
		vmaUnmapMemory(this->resources->allocator, this->resources->memory);
        this->mapped = nullptr;
    }
}

void StarBuffers::Buffer::writeToBuffer(void *data, vk::DeviceSize size, vk::DeviceSize offset)
{
    assert(this->mapped && "Cannot copy to unmapped buffer");

    if (size == vk::WholeSize)
    {
        memcpy(this->mapped, data, this->bufferSize);
    }
    else
    {
        char *memOffset = (char *)mapped;
        memOffset += offset;
        memcpy(memOffset, data, size);
    }
}

vk::Result StarBuffers::Buffer::flush(vk::DeviceSize size, vk::DeviceSize offset)
{
	auto result = vmaFlushAllocation(this->resources->allocator, this->resources->memory, offset, size); 
    return vk::Result(result);
}

vk::DescriptorBufferInfo StarBuffers::Buffer::descriptorInfo(vk::DeviceSize size, vk::DeviceSize offset)
{
    return vk::DescriptorBufferInfo{this->resources->buffer, offset, size};
}

void StarBuffers::Buffer::writeToIndex(void *data, int index)
{
    writeToBuffer(data, this->instanceSize, index * this->alignmentSize);
}

vk::Result StarBuffers::Buffer::flushIndex(int index)
{
    return flush(this->alignmentSize, index * this->alignmentSize);
}

vk::DescriptorBufferInfo StarBuffers::Buffer::descriptorInfoForIndex(int index)
{
    return descriptorInfo(this->alignmentSize, index * alignmentSize);
}

StarBuffers::Buffer::Buffer(VmaAllocator &allocator, const uint32_t &instanceCount, const vk::DeviceSize &instanceSize,
                            const vk::DeviceSize &minOffsetAlignment, const VmaAllocationCreateInfo &allocCreateInfo,
                            const vk::BufferCreateInfo &bufferCreateInfo, const std::string &allocName)
    : instanceCount(instanceCount), instanceSize(instanceSize),
      alignmentSize(GetAlignment(instanceSize, minOffsetAlignment)), bufferSize(bufferCreateInfo.size)
{
    VmaAllocationInfo nAllocInfo{};
    this->resources = CreateBuffer(allocator, allocCreateInfo, bufferCreateInfo, allocName, nAllocInfo);

    this->allocationInfo = std::make_optional<VmaAllocationInfo>(nAllocInfo);
}

std::shared_ptr<star::StarBuffers::Resources> StarBuffers::Buffer::CreateBuffer(
    VmaAllocator &allocator, const vk::DeviceSize &size, const vk::BufferUsageFlags &usage,
    const VmaMemoryUsage &memoryUsage, const VmaAllocationCreateFlags &flags, VmaAllocationInfo &allocationInfo,
    const std::string &allocationName)
{
    vk::Buffer buffer;
    VmaAllocation memory;

    vk::BufferCreateInfo bufferInfo{};
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = memoryUsage;
    allocInfo.flags = flags;

    auto result = (vk::Result)vmaCreateBuffer(allocator, (VkBufferCreateInfo *)&bufferInfo, &allocInfo,
                                              (VkBuffer *)&buffer, &memory, &allocationInfo);

    if (result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to allocate buffer memory");

    std::string fullAllocationName = std::string(allocationName);
    fullAllocationName += "_BUFFER";
    vmaSetAllocationName(allocator, memory, fullAllocationName.c_str());

    return std::make_shared<StarBuffers::Resources>(allocator, memory, buffer);
}

std::shared_ptr<star::StarBuffers::Resources> StarBuffers::Buffer::CreateBuffer(
    VmaAllocator &allocator, const VmaAllocationCreateInfo &allocCreateInfo, const vk::BufferCreateInfo &bufferInfo,
    const std::string &allocationName, VmaAllocationInfo &allocationInfo)
{
    vk::Buffer buffer;
    VmaAllocation memory;

    auto result = (vk::Result)vmaCreateBuffer(allocator, (VkBufferCreateInfo *)&bufferInfo, &allocCreateInfo,
                                              (VkBuffer *)&buffer, &memory, &allocationInfo);

    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to allocate buffer memory");
    }

    vmaSetAllocationName(allocator, memory, allocationName.c_str());

    return std::make_shared<StarBuffers::Resources>(allocator, memory, buffer);
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

std::unique_ptr<StarBuffers::Buffer> StarBuffers::Buffer::Builder::build()
{
    assert(this->instanceCount != 0 && this->instanceSize != 0 && "Instance info must be provided");

    return std::unique_ptr<StarBuffers::Buffer>(
        new StarBuffers::Buffer(this->allocator, this->instanceCount, this->instanceSize, this->minOffsetAlignment,
                                this->allocInfo, this->buffInfo, this->allocName));
}
} // namespace star