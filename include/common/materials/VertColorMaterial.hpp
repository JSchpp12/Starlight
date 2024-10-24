#pragma once

#include "StarMaterial.hpp"
#include "StarDevice.hpp"
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
		void buildDescriptorSet(StarDevice& device, StarShaderInfo::Builder& builder, const int& imageInFlightIndex) override;

	protected:
		void cleanup(StarDevice& device) override;
		void prep(StarDevice& device);

		// Inherited via StarMaterial
		void createDescriptors(star::StarDevice& device, const int& numFramesInFlight) override;
	};
}