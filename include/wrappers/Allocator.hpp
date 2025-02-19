#pragma once 

#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>

#include <memory>
#include <assert.h>
#include <iostream>
#include <vector>

namespace star {
	class Allocator {
	public:
		Allocator() = default;
		Allocator(vk::Device device, vk::PhysicalDevice physicalDevice, vk::Instance instance);
		~Allocator();

		VmaAllocator& get() {
			return this->allocator; 
		}

	protected:
		VmaAllocator allocator = VmaAllocator();

	};

}