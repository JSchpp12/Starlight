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

StarBuffer::StarBuffer(StarDevice& device, vk::DeviceSize instanceSize, uint32_t instanceCount,
	const VmaAllocationCreateFlags& creationFlags, const VmaMemoryUsage& memoryUsageFlags,
	const vk::BufferUsageFlags& useFlags, const vk::SharingMode& sharingMode,
	vk::DeviceSize minOffsetAlignment) :
	starDevice(device),
	usageFlags{useFlags},
	instanceSize{ instanceSize },
	instanceCount{ instanceCount },
	memoryPropertyFlags{ memoryPropertyFlags } {
	this->alignmentSize = getAlignment(this->instanceSize, minOffsetAlignment);
	this->bufferSize = this->alignmentSize * instanceCount;

	VmaAllocationInfo allocationInfo{}; 
	createBuffer(device, this->bufferSize, useFlags, memoryUsageFlags, creationFlags, this->buffer, this->memory, allocationInfo);

	this->allocationInfo = std::make_unique<VmaAllocationInfo>(allocationInfo);
	//this->starDevice.createBuffer(this->bufferSize, this->usageFlags, this->memoryPropertyFlags, this->buffer, this->memory);
}

StarBuffer::~StarBuffer() {
	if (mapped)
		vmaUnmapMemory(this->starDevice.getAllocator(), this->memory);

	vmaDestroyBuffer(this->starDevice.getAllocator(), this->buffer, this->memory);
}

void StarBuffer::map(vk::DeviceSize size, vk::DeviceSize offset) {
	assert(this->buffer && this->memory && "Called map on buffer before creation");

	vmaMapMemory(this->starDevice.getAllocator(), this->memory, &this->mapped);
}

void StarBuffer::unmap() {
	if (mapped) {
		vmaUnmapMemory(this->starDevice.getAllocator(), this->memory);
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
	auto result = vmaFlushAllocation(this->starDevice.getAllocator(), this->memory, offset, size);
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

void StarBuffer::createBuffer(StarDevice& device, vk::DeviceSize size, vk::BufferUsageFlags usage, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags flags, vk::Buffer& buffer, VmaAllocation& memory, VmaAllocationInfo& allocationInfo)
{
	vk::BufferCreateInfo bufferInfo{};
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = vk::SharingMode::eExclusive;

	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = memoryUsage;
	allocInfo.flags = flags;

	vmaCreateBuffer(device.getAllocator(), (VkBufferCreateInfo*)&bufferInfo, &allocInfo, (VkBuffer*)&buffer, &memory, &allocationInfo);
}

}