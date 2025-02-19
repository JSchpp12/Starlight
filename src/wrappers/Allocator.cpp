#include "Allocator.hpp"

star::Allocator::Allocator(vk::Device device, vk::PhysicalDevice physicalDevice, vk::Instance instance)
{
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.flags = 
		VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT & VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT & VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT & 
		VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT & VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT;
	allocatorInfo.device = device; 
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
	allocatorInfo.physicalDevice = physicalDevice; 
	allocatorInfo.instance = instance; 

	vmaCreateAllocator(&allocatorInfo, &this->allocator);
}

star::Allocator::~Allocator()
{
	vmaDestroyAllocator(this->allocator);
}
