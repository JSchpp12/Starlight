#pragma once

#include "StarMaterial.hpp"

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
			const int& shiny,Texture& texture, Texture& bumpMap) :
			surfaceColor(surfaceColor), highlightColor(highlightColor),
			ambient(ambient), diffuse(diffuse),
			specular(specular), shinyCoefficient(shiny), 
			texture(texture), bumpMap(bumpMap) {};

		// Inherited via StarMaterial
		void prepRender(StarDevice& device) override;
		void initDescriptorLayouts(StarDescriptorSetLayout::Builder& constBuilder) override;
		void buildConstDescriptor(StarDescriptorWriter writer) override;
		void bind(vk::CommandBuffer& commandBuffer, vk::PipelineLayout pipelineLayout, int swapChainImageIndex) override;


		glm::vec4 surfaceColor{ 0.5f, 0.5f, 0.5f, 1.0f };
		glm::vec4 highlightColor{ 0.5f, 0.5f, 0.5f, 1.0f };
		glm::vec4 ambient{ 0.5f, 0.5f, 0.5f, 1.0f };
		glm::vec4 diffuse{ 0.5f, 0.5f, 0.5f, 1.0f };
		glm::vec4 specular{ 0.5f, 0.5f, 0.5f, 1.0f };
		int shinyCoefficient = 1;

	protected:
		std::unique_ptr<StarTexture> renderTexture, renderBumpMap; 
		Texture& texture;
		Texture& bumpMap;

	};
}