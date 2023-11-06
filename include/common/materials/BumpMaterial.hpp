#pragma once

#include "StarMaterial.hpp"
#include "Texture.hpp"
#include "StarEngine.hpp"

#include "Handle.hpp"

#include <glm/glm.hpp>

#include <vulkan/vulkan.hpp>

#include <memory>
#include <optional>

namespace star {
	class BumpMaterial : public StarMaterial {
	public:
		BumpMaterial(const glm::vec4& surfaceColor, const glm::vec4& highlightColor, 
			const glm::vec4& ambient, const glm::vec4& diffuse, const glm::vec4& specular,
			const int& shiny,std::unique_ptr<Texture> texture, std::unique_ptr<Texture> bumpMap) :
			surfaceColor(surfaceColor), highlightColor(highlightColor),
			ambient(ambient), diffuse(diffuse),
			specular(specular), shinyCoefficient(shiny), 
			texture(std::move(texture)), bumpMap(std::move(bumpMap)) {};

		void prepRender(StarDevice& device) override;
		void getDescriptorSetLayout(star::StarDescriptorSetLayout::Builder& newLayout) override;
		void cleanupRender(StarDevice& device) override;

		glm::vec4 surfaceColor{ 0.5f, 0.5f, 0.5f, 1.0f };
		glm::vec4 highlightColor{ 0.5f, 0.5f, 0.5f, 1.0f };
		glm::vec4 ambient{ 0.5f, 0.5f, 0.5f, 1.0f };
		glm::vec4 diffuse{ 0.5f, 0.5f, 0.5f, 1.0f };
		glm::vec4 specular{ 0.5f, 0.5f, 0.5f, 1.0f };
		int shinyCoefficient = 1;

	protected:
		std::unique_ptr<Texture> texture, bumpMap; 

		vk::DescriptorSet buildDescriptorSet(StarDevice& device, StarDescriptorSetLayout& groupLayout,
			StarDescriptorPool& groupPool) override;

	};
}