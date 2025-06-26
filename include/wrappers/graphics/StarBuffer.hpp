#pragma once 

#include "Allocator.hpp"
#include "Enums.hpp"
#include "StarDevice.hpp"

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>

#include <string>

namespace star {

class StarBuffer {
public:
	class Builder{
		public:
			Builder(VmaAllocator &allocator);
			Builder& setAllocationCreateInfo(const VmaAllocationCreateInfo &nAllocInfo, const vk::BufferCreateInfo &nBufferInfo, const std::string &nAllocName); 
			Builder& setInstanceCount(const uint32_t &nInstanceCount); 
			Builder& setInstanceSize(const vk::DeviceSize &nInstanceSize); 
			Builder& setMinOffsetAlignment(const vk::DeviceSize &nMinOffsetAlignment);
			std::unique_ptr<StarBuffer> build(); 

		private:
			VmaAllocator &allocator; 
			vk::DeviceSize minOffsetAlignment = 1;
			std::string allocName; 
			VmaAllocationCreateInfo allocInfo; 
			vk::BufferCreateInfo buffInfo; 
			vk::DeviceSize instanceSize = 0;
			uint32_t instanceCount = 0;  
	};
	struct BufferCreationArgs {
		vk::DeviceSize instanceSize;
		uint32_t instanceCount;
		VmaAllocationCreateFlags creationFlags;
		VmaMemoryUsage memoryUsageFlags;
		vk::BufferUsageFlags useFlags;
		vk::SharingMode sharingMode;
		std::vector<uint32_t> queueIndices; 
		vk::DeviceSize minOffsetAlignment = 1; 
		std::string allocationName = "BufferDefaultName";

		BufferCreationArgs() = default;

		BufferCreationArgs(const vk::DeviceSize& instanceSize,
			const uint32_t& instanceCount, const VmaAllocationCreateFlags& creationFlags, const VmaMemoryUsage& memoryUsageFlags,
			const vk::BufferUsageFlags& useFlags, const vk::SharingMode& sharingMode, const std::string& allocationName, 
			const vk::DeviceSize& minOffsetAlignment = 1) 
			: instanceSize(instanceSize), instanceCount(instanceCount), creationFlags(creationFlags), memoryUsageFlags(memoryUsageFlags),
			useFlags(useFlags), sharingMode(sharingMode), allocationName(allocationName), minOffsetAlignment(minOffsetAlignment){};
	};

	static vk::DeviceSize GetAlignment(vk::DeviceSize instanceSize, vk::DeviceSize minOffsetAlignment);

	StarBuffer(VmaAllocator& allocator, vk::DeviceSize instanceSize, uint32_t instanceCount,
		const VmaAllocationCreateFlags& creationFlags, const VmaMemoryUsage& memoryUsageFlags,
		const vk::BufferUsageFlags& useFlags, const vk::SharingMode& sharingMode, const std::string& allocationName, 
		vk::DeviceSize minOffsetAlignment = 1);

	StarBuffer(VmaAllocator& allocator, const BufferCreationArgs& creationArgs); 

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

	vk::Buffer getVulkanBuffer() const { return buffer; }
	void* getMappepMemory() const { return mapped; }
	uint32_t getInstanceCount() const {return instanceCount;}
	vk::DeviceSize getInstanceSize() const { return instanceSize; }
	vk::DeviceSize getAlignmentSize() const { return alignmentSize; }
	vk::BufferUsageFlags getUsageFlags() const { return usageFlags; }
	vk::DeviceSize getBufferSize() const { return bufferSize; }

protected:
	VmaAllocator allocator = VmaAllocator();
	void* mapped = nullptr;
	VmaAllocation memory = VmaAllocation();

	vk::Buffer buffer = VK_NULL_HANDLE;

	vk::DeviceSize bufferSize;
	vk::DeviceSize instanceSize, alignmentSize;
	uint32_t instanceCount; 
	vk::BufferUsageFlags usageFlags;

	StarBuffer(VmaAllocator &allocator, const uint32_t &instanceCount, 
		const vk::DeviceSize &instanceSize, const vk::DeviceSize &minOffsetAlignment, 
		const VmaAllocationCreateInfo &allocCreateInfo, 
		const vk::BufferCreateInfo &bufferCreateInfo, const std::string &allocName);

	std::unique_ptr<VmaAllocationInfo> allocationInfo = nullptr;

	static void CreateBuffer(VmaAllocator& allocator, const vk::DeviceSize& size, 
		const vk::BufferUsageFlags& usage, const VmaMemoryUsage& memoryUsage, const VmaAllocationCreateFlags& flags, 
		vk::Buffer& buffer, VmaAllocation& memory, VmaAllocationInfo& allocationInfo, const std::string& allocationName);

	static void CreateBuffer(VmaAllocator &allocator, const VmaAllocationCreateInfo &allocCreateInfo, 
		const vk::BufferCreateInfo &bufferInfo, const std::string &allocationName, 
		vk::Buffer &buffer, VmaAllocation &memory, VmaAllocationInfo &allocationInfo);

	friend class Builder; 
};
}