#pragma once

#include "RenderResourceSystem.hpp"
#include "StarDevice.hpp"
#include "StarBuffer.hpp"

#include <functional>
#include <memory>

namespace star {
	class RenderResourceModifier {
	public:
		RenderResourceModifier() {
			this->registerCallbacks(); 
		}

		virtual void initResources(StarDevice& device, const int numFramesInFlight) = 0;

		virtual void destroyResources(StarDevice& device) = 0; 

	private:
		void registerCallbacks(); 

	};
}