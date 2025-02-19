#include "StarBuffer.hpp"
/*
* Initially based off Ive_buffer by Brendan Galea -
*https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanBuffer.h
*/

namespace star {
	vk::DeviceSize StarBuffer::getAlignment(vk::DeviceSize instanceSize, vk::DeviceSize minOffsetAlignment) {
	if (minOffsetAlignment > 0) {
		return  (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
	}
	return instanceSize;
}

StarBuffer::StarBuffer(Allocator& allocator, vk::DeviceSize instanceSize, uint32_t instanceCount,
	const VmaAllocationCreateFlags& creationFlags, const VmaMemoryUsage& memoryUsageFlags,
	const vk::BufferUsageFlags& useFlags, const vk::SharingMode& sharingMode,
	vk::DeviceSize minOffsetAlignment) :
	allocator(allocator),
	usageFlags{useFlags},
	instanceSize{ instanceSize },
	instanceCount{ instanceCount },
	memoryPropertyFlags{ memoryPropertyFlags } {

	this->alignmentSize = getAlignment(this->instanceSize, minOffsetAlignment);
	this->bufferSize = this->alignmentSize * instanceCount;

	if (bufferSize == 0)
		throw std::runtime_error("Unable to create a buffer of size 0");

	VmaAllocationInfo allocationInfo{}; 
	createBuffer(allocator, this->bufferSize, useFlags, memoryUsageFlags, creationFlags, this->buffer, this->memory, allocationInfo);

	this->allocationInfo = std::make_unique<VmaAllocationInfo>(allocationInfo);
}

StarBuffer::~StarBuffer() {
	if (mapped)
		vmaUnmapMemory(this->allocator.get(), this->memory);

	vmaDestroyBuffer(this->allocator.get(), this->buffer, this->memory);
}

void StarBuffer::map(vk::DeviceSize size, vk::DeviceSize offset) {
	assert(this->buffer && this->memory && "Called map on buffer before creation");

	vmaMapMemory(this->allocator.get(), this->memory, &this->mapped);
}

void StarBuffer::unmap() {
	if (mapped) {
		vmaUnmapMemory(this->allocator.get(), this->memory);
		this->mapped = nullptr;
	}
}

void StarBuffer::writeToBuffer(void* data, vk::DeviceSize size, vk::DeviceSize offset) {
	assert(this->mapped && "Cannot copy to unmapped buffer");

	if (size == vk::WholeSize) {
		memcpy(this->mapped, data, this->bufferSize);
	}
	else {
		char* memOffset = (char*)mapped;
		memOffset += offset;
		memcpy(memOffset, data, size);
	}
}

vk::Result StarBuffer::flush(vk::DeviceSize size, vk::DeviceSize offset) {
	auto result = vmaFlushAllocation(this->allocator.get(), this->memory, offset, size);
	return vk::Result(result);
}

vk::DescriptorBufferInfo StarBuffer::descriptorInfo(vk::DeviceSize size, vk::DeviceSize offset) {
	return vk::DescriptorBufferInfo{
		buffer,
		offset,
		size
	};
}

void StarBuffer::writeToIndex(void* data, int index) {
	writeToBuffer(data, this->instanceSize, index * this->alignmentSize);
}

vk::Result StarBuffer::flushIndex(int index) {
	return flush(this->alignmentSize, index * this->alignmentSize);
}

vk::DescriptorBufferInfo StarBuffer::descriptorInfoForIndex(int index) {
	return descriptorInfo(this->alignmentSize, index * alignmentSize);
}

void StarBuffer::createBuffer(Allocator& allocator, vk::DeviceSize size, vk::BufferUsageFlags usage, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags flags, vk::Buffer& buffer, VmaAllocation& memory, VmaAllocationInfo& allocationInfo)
{
	vk::BufferCreateInfo bufferInfo{};
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = vk::SharingMode::eExclusive;

	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = memoryUsage;
	allocInfo.flags = flags;

	vmaCreateBuffer(allocator.get(), (VkBufferCreateInfo*)&bufferInfo, &allocInfo, (VkBuffer*)&buffer, &memory, &allocationInfo);
}

}