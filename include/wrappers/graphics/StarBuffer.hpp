#pragma once 

#include "StarDevice.hpp"
#include "Enums.hpp"

#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>

namespace star {

class StarBuffer {
public:
	static vk::DeviceSize getAlignment(vk::DeviceSize instanceSize, vk::DeviceSize minOffsetAlignment);

	StarBuffer(StarDevice& device, vk::DeviceSize instanceSize, uint32_t instanceCount,
		const VmaAllocationCreateFlags& creationFlags, const VmaMemoryUsage& memoryUsageFlags,
		const vk::BufferUsageFlags& useFlags, const vk::SharingMode& sharingMode,
		vk::DeviceSize minOffsetAlignment = 1);
	~StarBuffer();

	//prevent copying
	StarBuffer(const StarBuffer&) = delete;
	StarBuffer& operator=(const StarBuffer&) = delete;

	void map(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);

	void unmap();

	void writeToBuffer(void* data, vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);

	vk::Result flush(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);

	vk::DescriptorBufferInfo descriptorInfo(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0);

	void writeToIndex(void* data, int index);
	vk::Result flushIndex(int index);
	vk::DescriptorBufferInfo descriptorInfoForIndex(int index);

	vk::Buffer getBuffer() const { return buffer; }
	void* getMappepMemory() const { return mapped; }
	uint32_t getInstanceCount() const { return instanceCount; }
	vk::DeviceSize getInstanceSize() const { return instanceSize; }
	vk::DeviceSize getAlignmentSize() const { return alignmentSize; }
	vk::BufferUsageFlags getUsageFlags() const { return usageFlags; }
	vk::MemoryPropertyFlags getMemoryPropertyFlags() const { return memoryPropertyFlags; }
	vk::DeviceSize getBufferSize() const { return bufferSize; }

private:
	StarDevice& starDevice;
	void* mapped = nullptr;
	VmaAllocation memory = VmaAllocation();

	vk::Buffer buffer = VK_NULL_HANDLE;

	vk::DeviceSize bufferSize;
	uint32_t instanceCount;
	vk::DeviceSize instanceSize, alignmentSize;
	vk::BufferUsageFlags usageFlags;
	vk::MemoryPropertyFlags memoryPropertyFlags;

	std::unique_ptr<VmaAllocationInfo> allocationInfo = nullptr;


	static void createBuffer(StarDevice& device, vk::DeviceSize size, 
		vk::BufferUsageFlags usage, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags flags, 
		vk::Buffer& buffer, VmaAllocation& memory, VmaAllocationInfo& allocationInfo);
};
}