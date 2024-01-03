#pragma once 

#include "RenderResourceModifier.hpp"
#include "RenderResourceSystem.hpp"
#include "StarBuffer.hpp"
#include "StarDevice.hpp"
#include "Handle.hpp"

#include <memory>
#include <functional>

namespace star {
	class RenderResourceModifierGeometry : public RenderResourceModifier{
	public:
		RenderResourceModifierGeometry() {
			registerCallbacks(); 
		}

		virtual std::pair<std::unique_ptr<StarBuffer>, std::unique_ptr<StarBuffer>> loadGeometryStagingBuffers(StarDevice& device, Handle& primaryVertBuffer, Handle& primaryIndexBuffer) = 0;
	private:
		void registerCallbacks(); 
	};
}