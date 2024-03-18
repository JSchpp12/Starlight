#pragma once

#include <vulkan/vulkan.hpp>

#include <memory>
#include <assert.h>
#include <iostream>
#include <vector>

#include "vk_mem_alloc.h"

namespace star {
	class Allocator {
	public:
		Allocator(const vk::Device& device, const vk::PhysicalDevice& physicalDevice, const vk::Instance& instance);
		~Allocator();

		VmaAllocator& get() {
			return *this->allocator; 
		}

	protected:
		std::unique_ptr<VmaAllocator> allocator = std::unique_ptr<VmaAllocator>(new VmaAllocator());

	};

}