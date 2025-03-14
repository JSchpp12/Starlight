#pragma once

#include "TextureMaterial.hpp"

#include "Handle.hpp"

#include <glm/glm.hpp>


namespace star {
	class BumpMaterial : public TextureMaterial {
	public:
		BumpMaterial(const glm::vec4& surfaceColor, const glm::vec4& highlightColor, 
			const glm::vec4& ambient, const glm::vec4& diffuse, const glm::vec4& specular,
			const int& shiny, const Handle& surfaceTexture, const Handle& bumpMap ) :
			TextureMaterial(surfaceColor, highlightColor, ambient, diffuse, specular, shiny, surfaceTexture), 
			bumpMap(bumpMap) {};


		void applyDescriptorSetLayouts(star::StarDescriptorSetLayout::Builder& constBuilder) override;
		void cleanup(StarDevice& device) override;

	protected:
		Handle bumpMap; 

		void buildDescriptorSet(StarDevice& device, StarShaderInfo::Builder& builder, const int& imageInFlightIndex) override;

		virtual std::vector<std::pair<vk::DescriptorType, const int>> getDescriptorRequests(const int& numFramesInFlight) override;
	};
}