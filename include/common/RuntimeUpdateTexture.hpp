#pragma once

#include "Texture.hpp"
#include "StarDevice.hpp"

#include <vulkan/vulkan.hpp>

namespace star {
	class RuntimeUpdateTexture : public Texture {
	public:
        RuntimeUpdateTexture(int texWidth, int texHeight) : Texture(texWidth, texHeight) {}; 
        RuntimeUpdateTexture(int texWidth, int texHeight, std::vector<std::vector<Color>> rawData) : Texture(texWidth, texHeight, rawData) {};

        virtual void prepRender(StarDevice& device) override; 

        /// <summary>
        /// Update the texture on the GPU. 
        /// Should only be done after the prepRender phase. 
        /// </summary>
        void updateGPU();

	protected:
        StarDevice* device = nullptr; 

	};
}