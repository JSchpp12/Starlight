#pragma once 

#include "StarDevice.hpp"
#include "StarBuffer.hpp"
#include "Handle.hpp"
#include "Vertex.hpp"

#include "BufferHandle.hpp"
#include "StarCommandBuffer.hpp"

#include <vulkan/vulkan.hpp>

#include <stack>
#include <functional>
#include <memory>
#include <vector>

namespace star {
	class RenderResourceSystem {
	public:
		static void registerCallbacks(std::function<void(StarDevice&, const int)> initCallback, std::function<void(StarDevice&)> destroyCallback);

		static void registerLoadGeomDataCallback(std::function<std::pair<std::unique_ptr<StarBuffer>, std::unique_ptr<StarBuffer>>(StarDevice&, Handle&, Handle&)>);

		static void bind(const Handle& resource, vk::CommandBuffer& commandBuffer);

		friend class StarEngine;
	private: 
		static std::stack<std::function<void(StarDevice&, const int)>> initCallbacks;
		static std::stack<std::function<void(StarDevice&)>> destroyCallbacks;
		static std::stack<std::function<std::pair<std::unique_ptr<StarBuffer>, std::unique_ptr<StarBuffer>>(StarDevice&, Handle&, Handle&)>> loadGeometryCallbacks;
		static std::vector<std::unique_ptr<StarBuffer>> oneTimeBuffers; 
		static std::vector<std::unique_ptr<StarBuffer>> buffers; 

		static void preparyPrimaryGeometry(StarDevice& device);

		static void runInits(StarDevice& device, const int numFramesInFlight);

		static void runDestroys(StarDevice& device); 
	};
}