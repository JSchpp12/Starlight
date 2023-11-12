#pragma once

#include "StarMaterial.hpp"
#include "Texture.hpp"

namespace star {
	class TextureMaterial : public StarMaterial {
	public:
		TextureMaterial(std::shared_ptr<Texture> texture) : texture(texture) {};

		TextureMaterial(const glm::vec4& surfaceColor, const glm::vec4& highlightColor,
			const glm::vec4& ambient, const glm::vec4& diffuse, const glm::vec4& specular,
			const int& shiny, std::shared_ptr<Texture> texture) 
			: StarMaterial(surfaceColor, highlightColor, ambient, diffuse, specular, shiny),
			texture(texture) {};

		void getDescriptorSetLayout(StarDescriptorSetLayout::Builder& newLayout) override;
		void cleanup(StarDevice& device) override;
		vk::DescriptorSet buildDescriptorSet(StarDevice& device, StarDescriptorSetLayout& groupLayout, StarDescriptorPool& groupPool) override;

	protected:
		std::shared_ptr<Texture> texture;

		void prep(StarDevice& device) override;
	};
}