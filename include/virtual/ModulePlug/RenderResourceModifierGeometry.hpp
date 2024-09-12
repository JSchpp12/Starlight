#pragma once 

#include "RenderResourceModifier.hpp"
#include "RenderResourceSystem.hpp"
#include "StarBuffer.hpp"
#include "StarDevice.hpp"
#include "BufferHandle.hpp"

#include <memory>
#include <functional>

namespace star {
	class RenderResourceModifierGeometry : public RenderResourceModifier {
	public:
		RenderResourceModifierGeometry() {
			registerRenderResourceCallbacks(); 
		}

		/// @brief Function to create staging buffers for geometry data
		/// @param device 
		/// @param primaryVertBuffer 
		/// @param primaryIndexBuffer 
		/// @return 
		virtual std::pair<std::unique_ptr<StarBuffer>, std::unique_ptr<StarBuffer>> loadGeometryStagingBuffers(StarDevice& device, BufferHandle primaryVertBuffer, BufferHandle primaryIndexBuffer) = 0;

		void setGeometryBufferOffsets(const uint32_t& vertOffset, const uint32_t& indOffset, const uint32_t& numInds) {
			this->vertBufferOffset = vertOffset; 
			this->numIndices = numInds;
			this->indBufferOffset = indOffset;
		};

	protected: 
		uint32_t vertBufferOffset=0, indBufferOffset=0, numIndices=0;

	private:
		void registerRenderResourceCallbacks();
	};
}