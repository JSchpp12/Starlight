#pragma once

#include "Color.hpp"
#include "StarMaterial.hpp"
#include "Texture.hpp"
#include "RuntimeUpdateTexture.hpp"

#include <memory>
#include <vector>

namespace star {
	class HeightDisplacementMaterial : public StarMaterial {
	public:
		HeightDisplacementMaterial(int size_x, int size_y) 
			: size_x(size_x), size_y(size_y), displaceTexture(size_x, size_y) {};

		// Inherited via StarMaterial
		void prepRender(StarDevice& device) override;

		void initDescriptorLayouts(StarDescriptorSetLayout::Builder& constBuilder) override;

		void buildConstDescriptor(StarDescriptorWriter writer) override;

		void bind(vk::CommandBuffer& commandBuffer, vk::PipelineLayout pipelineLayout, int swapChainImageIndex) override;

		RuntimeUpdateTexture& getTexture() { return this->displaceTexture; }

	protected:
		RuntimeUpdateTexture displaceTexture; 
		int size_x = 0, size_y = 0;

	};
}