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
				this->myInfo.flags = nFlags;
				return *this;
			}; 
			AllocationBuilder& setUsage(const VmaMemoryUsage& nUsage){
				this->myInfo.usage = nUsage;
				return *this;
			}
			AllocationBuilder& setMemoryRequiredFlags(const vk::MemoryPropertyFlags& nRequiredFlags){
				this->myInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(vk::MemoryPropertyFlags(nRequiredFlags)); 
				return *this;
			}
			AllocationBuilder& setMemoryPreferredFlags(const vk::MemoryPropertyFlags& nPreferredFlags){
				this->myInfo.preferredFlags = static_cast<VkMemoryPropertyFlags>(vk::MemoryPropertyFlags(nPreferredFlags)); 
				return *this;
			}
			AllocationBuilder& setMemoryTypeBits(const uint32_t& nMemoryTypeBits){
				this->myInfo.memoryTypeBits = nMemoryTypeBits;
				return *this;
			}
			AllocationBuilder& setPriority(const float& nPriority){
				this->myInfo.priority = nPriority; 
				return *this;
			}
			VmaAllocationCreateInfo build(){
				return this->myInfo; 
			}

			private:
			VmaAllocationCreateInfo myInfo = VmaAllocationCreateInfo(); 

		};
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