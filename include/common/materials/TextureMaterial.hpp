#pragma once

#include "StarMaterial.hpp"
#include "FileTexture.hpp"
#include "ManagerDescriptorPool.hpp"

namespace star {
	class TextureMaterial : public StarMaterial {
	public:
		TextureMaterial(std::shared_ptr<FileTexture> texture) : texture(texture) {};

		TextureMaterial(const glm::vec4& surfaceColor, const glm::vec4& highlightColor,
			const glm::vec4& ambient, const glm::vec4& diffuse, const glm::vec4& specular,
			const int& shiny, std::shared_ptr<FileTexture> texture) 
			: StarMaterial(surfaceColor, highlightColor, ambient, diffuse, specular, shiny),
			texture(texture) {};

		virtual void applyDescriptorSetLayouts(star::StarDescriptorSetLayout::Builder& constBuilder) override;
		void cleanup(StarDevice& device) override;
		void buildDescriptorSet(StarDevice& device, StarShaderInfo::Builder& builder, const int& imageInFlightIndex) override;

	protected:
		std::shared_ptr<FileTexture> texture;

		void prep(StarDevice& device) override;

		virtual void initResources(StarDevice& device, const int& numFramesInFlight, const vk::Extent2D& screensize) override;

		virtual std::vector<std::pair<vk::DescriptorType, const int>> getDescriptorRequests(const int& numFramesInFlight) override;

		// Inherited via StarMaterial
		void createDescriptors(star::StarDevice& device, const int& numFramesInFlight) override;

	};
}