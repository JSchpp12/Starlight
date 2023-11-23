#pragma once

#include "TextureMaterial.hpp"
#include "StarMaterial.hpp"
#include "Texture.hpp"
#include "StarEngine.hpp"

#include "Handle.hpp"

#include <glm/glm.hpp>

#include <vulkan/vulkan.hpp>

#include <memory>
#include <optional>

namespace star {
	class BumpMaterial : public TextureMaterial {
	public:
		BumpMaterial(const glm::vec4& surfaceColor, const glm::vec4& highlightColor, 
			const glm::vec4& ambient, const glm::vec4& diffuse, const glm::vec4& specular,
			const int& shiny,std::unique_ptr<Texture> texture, std::unique_ptr<Texture> bumpMap) :
			TextureMaterial(surfaceColor, highlightColor, ambient, diffuse, specular, shiny, std::move(texture)), 
			bumpMap(std::move(bumpMap)) {};


		void getDescriptorSetLayout(star::StarDescriptorSetLayout::Builder& newLayout) override;
		void cleanup(StarDevice& device) override;

	protected:
		std::unique_ptr<Texture> bumpMap; 

		vk::DescriptorSet buildDescriptorSet(StarDevice& device, StarDescriptorSetLayout& groupLayout,
			StarDescriptorPool& groupPool) override;

		void prep(StarDevice& device) override;

	};
}