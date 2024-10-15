#pragma once

#include "RenderResourceSystem.hpp"
#include "StarDevice.hpp"
#include "StarBuffer.hpp"
#include <functional>
#include <memory>
#include <vector>
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