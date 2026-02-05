#pragma once 

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>

#include <memory>
#include <assert.h>
#include <iostream>
#include <vector>

namespace star {
	class Allocator {
	public:
		class AllocationBuilder{
			public:
			AllocationBuilder() = default;
			AllocationBuilder& setFlags(const VmaAllocationCreateFlags& nFlags){
				this->flags = nFlags;
				return *this;
			}; 
			AllocationBuilder& setUsage(const VmaMemoryUsage& nUsage){
				this->usage = nUsage;
				return *this;
			}
			AllocationBuilder& setMemoryRequiredFlags(const vk::MemoryPropertyFlags& nRequiredFlags){
				this->requiredFlags = nRequiredFlags;
				return *this;
			}
			AllocationBuilder& setMemoryPreferredFlags(const vk::MemoryPropertyFlags& nPreferredFlags){
				this->preferredFlags = nPreferredFlags; 
				return *this;
			}
			AllocationBuilder& setMemoryTypeBits(const uint32_t& nMemoryTypeBits){
				this->memoryTypeBits = nMemoryTypeBits;
				return *this;
			}
			AllocationBuilder& setPriority(const float& nPriority){
				this->priority = nPriority; 
				return *this;
			}
			VmaAllocationCreateInfo build(){
				VmaAllocationCreateInfo info = VmaAllocationCreateInfo(); 
				info.flags = this->flags; 
				info.usage = this->usage; 
				info.preferredFlags = static_cast<VkMemoryPropertyFlags>(this->preferredFlags); 
				info.memoryTypeBits = this->memoryTypeBits;
				info.priority = this->priority; 

				return info; 
			}

			private:
            VmaAllocationCreateFlags flags{}; 
			VmaMemoryUsage usage{}; 
			vk::MemoryPropertyFlags requiredFlags{}, preferredFlags{};
			uint32_t memoryTypeBits = 0; 
			float priority = 0.0f; 
		};
		
		Allocator() = default;
		Allocator(vk::Device device, vk::PhysicalDevice physicalDevice, vk::Instance instance);

		VmaAllocator& get() {
			return this->allocator; 
		}

		void cleanupRender(); 

	protected:
		VmaAllocator allocator = VmaAllocator();
	};
}