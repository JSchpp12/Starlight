#pragma once

#include "StarMaterial.hpp"
#include "StarDevice.hpp"
#include "StarDescriptors.hpp"


namespace star {
	class VertColorMaterial : public StarMaterial {
	public:


		void getDescriptorSetLayout(StarDescriptorSetLayout::Builder& newLayout) override;
		vk::DescriptorSet buildDescriptorSet(StarDevice& device, StarDescriptorSetLayout& groupLayout, StarDescriptorPool& groupPool) override;

	protected:
		void cleanup(StarDevice& device) override;
		void prep(StarDevice& device);
	};
}