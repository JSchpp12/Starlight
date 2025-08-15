#pragma once

#include "StarMaterial.hpp"
#include "StarDescriptorBuilders.hpp"


namespace star {
	class VertColorMaterial : public StarMaterial {
	public:
		VertColorMaterial() = default; 

		VertColorMaterial(const glm::vec4& surfaceColor, const glm::vec4& highlightColor,
			const glm::vec4& ambient, const glm::vec4& diffuse, 
			const glm::vec4& specular, const int& shiny) 
			: StarMaterial(surfaceColor, highlightColor, ambient, diffuse, specular, shiny) {};

		virtual void applyDescriptorSetLayouts(star::StarDescriptorSetLayout::Builder& constBuilder) override;
		void buildDescriptorSet(core::DeviceContext& device, StarShaderInfo::Builder& builder, const int& imageInFlightIndex) override;

	protected:
		void cleanup(core::DeviceContext& device) override;
		void prep(core::DeviceContext& device) override;

		// Inherited via StarMaterial
		void createDescriptors(star::core::DeviceContext& device, const int& numFramesInFlight) override;
	};
}