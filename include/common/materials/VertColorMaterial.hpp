#pragma once

#include "StarMaterial.hpp"
#include "StarDevice.hpp"
#include "StarDescriptors.hpp"


namespace star {
	class VertColorMaterial : public StarMaterial {
	public:
		void prepRender(StarDevice& device);

		void getDescriptorSetLayout(StarDescriptorSetLayout::Builder& newLayout) override;
		void cleanupRender(StarDevice& device) override;
		vk::DescriptorSet buildDescriptorSet(StarDevice& device, StarDescriptorSetLayout& groupLayout, StarDescriptorPool& groupPool) override;
	};
}