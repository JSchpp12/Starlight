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

		void addDescriptorSetLayoutsTo(star::StarDescriptorSetLayout::Builder& constBuilder) const override;
		
	protected:

		std::unique_ptr<StarShaderInfo> buildShaderInfo(core::device::DeviceContext &context,
                                               const uint8_t &numFramesInFlight, StarShaderInfo::Builder builder) override; 
	};
}