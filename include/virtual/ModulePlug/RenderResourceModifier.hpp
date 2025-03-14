#pragma once

#include "StarDevice.hpp"

#include <vulkan/vulkan.hpp>

namespace star {
	class RenderResourceModifier {
	public:
		RenderResourceModifier() {
			this->registerRenderResourceCallbacks(); 
		}

		virtual void initResources(StarDevice& device, const int& numFramesInFlight, const vk::Extent2D& screenSize) = 0;

		virtual void destroyResources(StarDevice& device) = 0; 

	private:
		void registerRenderResourceCallbacks();

	};
}