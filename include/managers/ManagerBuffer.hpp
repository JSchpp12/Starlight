#pragma once

#include "Enums.hpp"
#include "StarBuffer.hpp"
#include "StarDevice.hpp"
#include "StarBuffer.hpp"
#include "Handle.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>
#include <map>
#include <array>
#include <queue>
#include <functional>
#include <stack>

#include <vma/vk_mem_alloc.h>

namespace star {
	class ManagerBuffer {
	public:
		struct Request {
			std::function<void(StarBuffer&)> updateBufferData = std::function<void(StarBuffer&)>(); 
			vk::DeviceSize bufferSize; 
			uint32_t instanceCount;
			VmaAllocationCreateFlags creationFlags;
			VmaMemoryUsage memoryUsageFlags;
			vk::BufferUsageFlags useFlags;
			vk::SharingMode sharingMode;
			int frameInFlightIndexToUpdateOn = -1; 
			vk::DeviceSize minOffsetAlignment = 1;

			Request( const std::function<void(StarBuffer&)>& updateBufferData, const vk::DeviceSize& bufferSize,
				const uint32_t& instanceCount, const VmaAllocationCreateFlags& creationFlags, const VmaMemoryUsage& memoryUsageFlags,
				const vk::BufferUsageFlags& useFlags, const vk::SharingMode& sharingMode, 
				const vk::DeviceSize& minOffsetAlignment, const int& frameInFlightIndexToUpdateOn)
				: updateBufferData(updateBufferData),
				bufferSize(bufferSize), instanceCount(instanceCount),
				creationFlags(creationFlags), memoryUsageFlags(memoryUsageFlags), 
				useFlags(useFlags), sharingMode(sharingMode), 
				frameInFlightIndexToUpdateOn(frameInFlightIndexToUpdateOn), 
				minOffsetAlignment(minOffsetAlignment) {};

			Request(const std::function<void(StarBuffer&)>& updateBufferData, const vk::DeviceSize& bufferSize,
				const uint32_t& instanceCount, const VmaAllocationCreateFlags& creationFlags, const VmaMemoryUsage& memoryUsageFlags,
				const vk::BufferUsageFlags& useFlags, const vk::SharingMode& sharingMode,
				const vk::DeviceSize& minOffsetAlignment)
				: updateBufferData(updateBufferData),
				bufferSize(bufferSize), instanceCount(instanceCount),
				creationFlags(creationFlags), memoryUsageFlags(memoryUsageFlags),
				useFlags(useFlags), sharingMode(sharingMode),
				minOffsetAlignment(minOffsetAlignment) {};

		};

		ManagerBuffer() = default;

		static void init(StarDevice& device, const int& totalNumFramesInFlight); 

		static Handle addRequest(const Request& newRequest);

		static void update(const int& frameInFlightIndex); 

		static StarBuffer& getBuffer(const Handle& handle); 

		static void cleanup(StarDevice& device); 

		static void destroy(const Handle& handle);

	protected:
		struct CompleteRequest {
			std::unique_ptr<StarBuffer> buffer;
			std::function<void(StarBuffer&)> updateBufferData = std::function<void(StarBuffer&)>();

			//1 - first frame
			//99 - done never again
			uint16_t frameInFlightIndexToUpdateOn; 

			CompleteRequest(std::unique_ptr<StarBuffer> buffer, 
				std::function<void(StarBuffer&)> updateBufferData, 
				const uint16_t& frameInFlightIndexToUpdateOn)
				: buffer(std::move(buffer)), updateBufferData(updateBufferData), 
				frameInFlightIndexToUpdateOn(frameInFlightIndexToUpdateOn) {};
		};

		static StarDevice* device;
		static int numFramesInFlight; 
		static int currentFrameInFlight; 
		static int bufferCounter; 

		//odd handles
		static std::unordered_map<Handle, CompleteRequest, HandleHash> updateableBuffers;
		//even handles
		static std::unordered_map<Handle, CompleteRequest, HandleHash> staticBuffers; 


		static std::unordered_map<Handle, CompleteRequest, HandleHash>* getMapForBuffer(const Handle& handle); 
	};
}