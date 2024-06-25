#pragma once

#include "StarMaterial.hpp"
#include "Texture.hpp"
#include "ManagerDescriptorPool.hpp"

namespace star {
	class TextureMaterial : public StarMaterial {
	public:
		TextureMaterial(std::shared_ptr<Texture> texture) : texture(texture) {};

		TextureMaterial(const glm::vec4& surfaceColor, const glm::vec4& highlightColor,
			const glm::vec4& ambient, const glm::vec4& diffuse, const glm::vec4& specular,
			const int& shiny, std::shared_ptr<Texture> texture) 
			: StarMaterial(surfaceColor, highlightColor, ambient, diffuse, specular, shiny),
			texture(texture) {};

		virtual void applyDescriptorSetLayouts(star::StarDescriptorSetLayout::Builder& constBuilder) override;
		void cleanup(StarDevice& device) override;
		vk::DescriptorSet buildDescriptorSet(StarDevice& device, StarDescriptorSetLayout& groupLayout, StarDescriptorPool& groupPool) override;

	protected:
		std::shared_ptr<Texture> texture;

		void prep(StarDevice& device) override;

		virtual void initResources(StarDevice& device, const int& numFramesInFlight) override;
	};
}