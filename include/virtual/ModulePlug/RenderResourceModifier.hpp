#pragma once

#include "RenderResourceSystem.hpp"

#include <functional>

namespace star {
	class RenderResourceModifier {
	public:
		RenderResourceModifier() {
			this->registerCallbacks(); 
		}

		virtual void initResources(int numFramesInFlight) = 0;

		virtual void registerCallbacks(); 
	protected:
		
	private:
	
	};
}