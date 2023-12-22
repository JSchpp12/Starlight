#pragma once

#include "StarMaterial.hpp"
#include "StarDevice.hpp"
#include "StarDescriptors.hpp"


namespace star {
	class VertColorMaterial : public StarMaterial {
	public:
		VertColorMaterial() = default; 

		VertColorMaterial(const glm::vec4& surfaceColor, const glm::vec4& highlightColor,
			const glm::vec4& ambient, const glm::vec4& diffuse, 
			const glm::vec4& specular, const int& shiny) 
			: StarMaterial(surfaceColor, highlightColor, ambient, diffuse, specular, shiny) {};

		virtual void applyDescriptorSetLayouts(star::StarDescriptorSetLayout::Builder& constBuilder) override;
		vk::DescriptorSet buildDescriptorSet(StarDevice& device, StarDescriptorSetLayout& groupLayout, StarDescriptorPool& groupPool) override;

	protected:
		void cleanup(StarDevice& device) override;
		void prep(StarDevice& device);
	};
}