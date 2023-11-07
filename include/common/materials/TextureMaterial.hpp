#pragma once

#include "StarMaterial.hpp"
#include "Texture.hpp"

namespace star {
	class TextureMaterial : public StarMaterial {
	public:
		TextureMaterial(const glm::vec4& surfaceColor, const glm::vec4& highlightColor,
			const glm::vec4& ambient, const glm::vec4& diffuse, const glm::vec4& specular,
			const int& shiny, std::unique_ptr<Texture> texture) 
			: StarMaterial(surfaceColor, highlightColor, ambient, diffuse, specular, shiny),
			texture(std::move(texture)) {};

		// Inherited via StarMaterial
		void getDescriptorSetLayout(StarDescriptorSetLayout::Builder& newLayout) override;
		void cleanup(StarDevice& device) override;
		vk::DescriptorSet buildDescriptorSet(StarDevice& device, StarDescriptorSetLayout& groupLayout, StarDescriptorPool& groupPool) override;

	protected:
		std::unique_ptr<Texture> texture;

		void prep(StarDevice& device) override;

	};
}