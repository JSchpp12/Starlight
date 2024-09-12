#pragma once 

#include "StarDevice.hpp"
#include "StarBuffer.hpp"
#include "BufferHandle.hpp"
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
		static void registerCallbacks(std::function<void(StarDevice&, const int)> initCallback, 
			std::function<void(StarDevice&)> destroyCallback);

		static void registerLoadGeomDataCallback(std::function<std::pair<std::unique_ptr<StarBuffer>, std::unique_ptr<StarBuffer>>(StarDevice&, BufferHandle, BufferHandle)>);

		static void registerSetDrawInfoCallback(std::function<void(const uint32_t&, const uint32_t&, const uint32_t&)> setGeometryDataOffsetCallback);

		static void bind(const Handle& resource, vk::CommandBuffer& commandBuffer);

		static void bind(const BufferHandle& buffer, vk::CommandBuffer& commandBuffer);

		static void init(StarDevice& device, const int& numFramesInFlight);

		static void cleanup(StarDevice& device);

		friend class StarEngine;
	private: 
		static std::stack<std::function<void(StarDevice&, const int)>> initCallbacks;
		static std::stack<std::function<void(StarDevice&)>> destroyCallbacks;
		static std::stack<std::function<std::pair<std::unique_ptr<StarBuffer>, std::unique_ptr<StarBuffer>>(StarDevice&, BufferHandle, BufferHandle)>> loadGeometryCallbacks;
		static std::stack<std::function<void(const uint32_t&, const uint32_t&, const uint32_t&)>> geometryDataOffsetCallbacks;
		static std::vector<std::unique_ptr<StarBuffer>> buffers; 

		static void preparePrimaryGeometry(StarDevice& device);

		static void runInits(StarDevice& device, const int numFramesInFlight);

		static void runDestroys(StarDevice& device); 

		static void bindBuffer(const uint32_t& bufferId, vk::CommandBuffer& commandBuffer, const size_t& offset=0);
	};
}