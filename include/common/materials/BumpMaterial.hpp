#pragma once

#include "TextureMaterial.hpp"
#include "StarMaterial.hpp"
#include "FileTexture.hpp"
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
			const int& shiny,std::unique_ptr<FileTexture> texture, std::unique_ptr<FileTexture> bumpMap) :
			TextureMaterial(surfaceColor, highlightColor, ambient, diffuse, specular, shiny, std::move(texture)), 
			bumpMap(std::move(bumpMap)) {};


		void applyDescriptorSetLayouts(star::StarDescriptorSetLayout::Builder& constBuilder) override;
		void cleanup(StarDevice& device) override;

	protected:
		std::unique_ptr<FileTexture> bumpMap; 

		void buildDescriptorSet(StarDevice& device, StarShaderInfo::Builder& builder, const int& imageInFlightIndex) override;

		void prep(StarDevice& device) override;

		virtual std::vector<std::pair<vk::DescriptorType, const int>> getDescriptorRequests(const int& numFramesInFlight) override;
	};
}