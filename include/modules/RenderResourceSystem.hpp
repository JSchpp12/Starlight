#pragma once 

#include "DeviceContext.hpp"
#include "StarBuffers/Buffer.hpp"
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
		static void registerCallbacks(std::function<void(core::DeviceContext&, const int&, const vk::Extent2D&)> initCallback, 
			std::function<void(core::DeviceContext&)> destroyCallback);

		static void registerLoadGeomDataCallback(std::function<std::pair<std::unique_ptr<StarBuffers::Buffer>, std::unique_ptr<StarBuffers::Buffer>>(core::DeviceContext&, BufferHandle, BufferHandle)>);

		static void registerSetDrawInfoCallback(std::function<void(const uint32_t&, const uint32_t&, const uint32_t&)> setGeometryDataOffsetCallback);

		static void bind(const Handle& resource, vk::CommandBuffer& commandBuffer);

		static void bind(const BufferHandle& buffer, vk::CommandBuffer& commandBuffer);

		static void init(core::DeviceContext& device, const int& numFramesInFlight, const vk::Extent2D& screensize);

		static void cleanup(core::DeviceContext& device);

		friend class StarEngine;
	private: 
		static std::stack<std::function<void(core::DeviceContext&, const int&, const vk::Extent2D&)>> initCallbacks;
		static std::stack<std::function<void(core::DeviceContext&)>> destroyCallbacks;
		static std::stack<std::function<std::pair<std::unique_ptr<StarBuffers::Buffer>, std::unique_ptr<StarBuffers::Buffer>>(core::DeviceContext&, BufferHandle, BufferHandle)>> loadGeometryCallbacks;
		static std::stack<std::function<void(const uint32_t&, const uint32_t&, const uint32_t&)>> geometryDataOffsetCallbacks;
		static std::vector<std::unique_ptr<StarBuffers::Buffer>> buffers; 

		static void preparePrimaryGeometry(core::DeviceContext& device);

		static void runInits(core::DeviceContext& device, const int& numFramesInFlight, const vk::Extent2D& screensize);

		static void runDestroys(core::DeviceContext& device); 

		static void bindBuffer(const uint32_t& bufferId, vk::CommandBuffer& commandBuffer, const size_t& offset=0);
	};
}