#pragma once

#include "StarMaterial.hpp"
#include "StarDevice.hpp"
#include "StarDescriptors.hpp"


namespace star {
	class VertColorMaterial : public StarMaterial {
	public:
		virtual void applyDescriptorSetLayouts(star::StarDescriptorSetLayout::Builder& constBuilder) override;
		vk::DescriptorSet buildDescriptorSet(StarDevice& device, StarDescriptorSetLayout& groupLayout, StarDescriptorPool& groupPool) override;

	protected:
		void cleanup(StarDevice& device) override;
		void prep(StarDevice& device);
	};
}