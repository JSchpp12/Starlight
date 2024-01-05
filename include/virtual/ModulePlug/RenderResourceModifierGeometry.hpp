#pragma once 

#include "RenderResourceModifier.hpp"
#include "RenderResourceSystem.hpp"
#include "StarBuffer.hpp"
#include "StarDevice.hpp"
#include "BufferHandle.hpp"

#include <memory>
#include <functional>

namespace star {
	class RenderResourceModifierGeometry : public RenderResourceModifier{
	public:
		RenderResourceModifierGeometry() {
			registerCallbacks(); 
		}

		virtual std::pair<std::unique_ptr<StarBuffer>, std::unique_ptr<StarBuffer>> loadGeometryStagingBuffers(StarDevice& device, BufferHandle primaryVertBuffer, BufferHandle primaryIndexBuffer) = 0;
	private:
		void registerCallbacks(); 
	};
}