#pragma once

#include "FileTexture.hpp"
#include "StarDevice.hpp"

#include <vulkan/vulkan.hpp>

namespace star {
	class RuntimeUpdateTexture : public FileTexture {
	public:
        RuntimeUpdateTexture(int texWidth, int texHeight) : FileTexture(texWidth, texHeight) {}; 
        RuntimeUpdateTexture(int texWidth, int texHeight, std::vector<std::vector<Color>> rawData) : FileTexture(texWidth, texHeight, rawData) {};

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