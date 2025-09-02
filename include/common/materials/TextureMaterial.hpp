#pragma once

#include "StarMaterial.hpp"
#include "Handle.hpp"

namespace star {
	class TextureMaterial : public StarMaterial {
	public:
		TextureMaterial(const Handle& texture) : texture(texture) {};

		TextureMaterial(const glm::vec4& surfaceColor, const glm::vec4& highlightColor,
			const glm::vec4& ambient, const glm::vec4& diffuse, const glm::vec4& specular,
			const int& shiny, const Handle& texture) 
			: StarMaterial(surfaceColor, highlightColor, ambient, diffuse, specular, shiny),
			texture(texture) {};

		virtual void applyDescriptorSetLayouts(star::StarDescriptorSetLayout::Builder& constBuilder) override;
		void cleanup(core::device::DeviceContext& device) override;
		void buildDescriptorSet(core::device::DeviceContext& device, StarShaderInfo::Builder& builder, const int& imageInFlightIndex) override;
		virtual std::vector<std::pair<vk::DescriptorType, const int>> getDescriptorRequests(const int& numFramesInFlight) override;

	protected:
		Handle texture;

		void prep(core::device::DeviceContext& device) override;

		virtual void initResources(core::device::DeviceContext& device, const int& numFramesInFlight, const vk::Extent2D& screensize) override;

		// Inherited via StarMaterial
		void createDescriptors(star::core::device::DeviceContext& device, const int& numFramesInFlight) override;

	};
}