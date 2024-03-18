#include "Allocator.hpp"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

star::Allocator::Allocator(const vk::Device& device, const vk::PhysicalDevice& physicalDevice, const vk::Instance& instance)
{
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.flags = 
		VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT & VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT & VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT &
		VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT & 
		VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT & VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT;
	allocatorInfo.device = device; 
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
	allocatorInfo.physicalDevice = physicalDevice; 
	allocatorInfo.instance = instance; 

	vmaCreateAllocator(&allocatorInfo, this->allocator.get());
}

star::Allocator::~Allocator()
{
	vmaDestroyAllocator(*this->allocator);
}
