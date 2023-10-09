#pragma once

#include "StarMaterial.hpp"
#include "StarDevice.hpp"
#include "StarDescriptors.hpp"


namespace star {
	class VertColorMaterial : public StarMaterial {
	public:

		void prepRender(StarDevice& device);

		void initDescriptorLayouts(StarDescriptorSetLayout::Builder& constBuilder); 

		void buildConstDescriptor(StarDescriptorWriter writer);

		void bind(vk::CommandBuffer& commandBuffer, vk::PipelineLayout pipelineLayout, int swapChainImageIndex); 
	};
}