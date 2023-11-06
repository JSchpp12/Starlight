#pragma once 

#include "StarDevice.hpp"

#include <vulkan/vulkan.hpp>

namespace star {
	class StarRenderPass {
	public:
		StarRenderPass(StarDevice& device) : device(device) {};
		virtual ~StarRenderPass(); 

	protected:
		StarDevice& device; 
		vk::RenderPass renderPass; 

	};
}