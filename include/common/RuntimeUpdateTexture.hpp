#pragma once

#include "Texture.hpp"

#include <vulkan/vulkan.hpp>

namespace star {
	class RuntimeUpdateTexture : public Texture {
	public:
        RuntimeUpdateTexture(int texWidth, int texHeight) : Texture(texWidth, texHeight) {}; 

        /// <summary>
        /// Update the texture on the GPU. 
        /// Should only be done after the prepRender phase. 
        /// </summary>
        void updateGPU();

	protected:


	};
}