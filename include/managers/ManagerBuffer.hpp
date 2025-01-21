#pragma once

#include "Enums.hpp"
#include "StarBuffer.hpp"
#include "StarDevice.hpp"
#include "StarManager.hpp"
#include "Handle.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>
#include <map>
#include <array>
#include <queue>
#include <functional>
#include <stack>
#include <optional>

#include <vma/vk_mem_alloc.h>

namespace star {
	class ManagerBuffer : public StarManager{
	public:
		struct BufferCreationArgs {
			vk::DeviceSize bufferSize;
			uint32_t instanceCount;
			VmaAllocationCreateFlags creationFlags;
			VmaMemoryUsage memoryUsageFlags;
			vk::BufferUsageFlags useFlags;
			vk::SharingMode sharingMode;
			vk::DeviceSize minOffsetAlignment = 1;

			BufferCreationArgs(const vk::DeviceSize& bufferSize,
				const uint32_t& instanceCount, const VmaAllocationCreateFlags& creationFlags, const VmaMemoryUsage& memoryUsageFlags,
				const vk::BufferUsageFlags& useFlags, const vk::SharingMode& sharingMode,
				const vk::DeviceSize& minOffsetAlignment) 
				: bufferSize(bufferSize), instanceCount(instanceCount), creationFlags(creationFlags), memoryUsageFlags(memoryUsageFlags),
				useFlags(useFlags), sharingMode(sharingMode), minOffsetAlignment(minOffsetAlignment) {};
		};

		struct Request {
			std::function<void(StarBuffer&)> updateBufferData = std::function<void(StarBuffer&)>(); 
			std::optional<std::function<bool(void)>> checkIfNeedsUpdate = std::optional<std::function<bool(void)>>(); 
			std::function<void(void)> setBufferAsChanged = std::function<void(void)>();
			BufferCreationArgs bufferCreateArgs;
			int frameInFlightIndexToUpdateOn = -1; 

			Request(const std::function<void(StarBuffer&)>& updateBufferData, const std::function<void(void)>& setBufferAsChanged, 
				const BufferCreationArgs& bufferCreateArgs, const int& frameInFlightIndexToUpdateOn, 
				std::function<bool(void)> checkIfNeedsUpdate)
				: updateBufferData(updateBufferData), setBufferAsChanged(setBufferAsChanged),
				bufferCreateArgs(bufferCreateArgs), frameInFlightIndexToUpdateOn(frameInFlightIndexToUpdateOn),
				checkIfNeedsUpdate(std::make_optional(checkIfNeedsUpdate)){};

			Request(const std::function<void(StarBuffer&)>& updateBufferData, 
				const std::function<void(void)>& setBufferAsChanged, 
				const BufferCreationArgs& bufferCreateArgs)
				: updateBufferData(updateBufferData), setBufferAsChanged(setBufferAsChanged),
				bufferCreateArgs(bufferCreateArgs) {};
		};

		ManagerBuffer(StarDevice& device);

		static void init(StarDevice& device, const int& totalNumFramesInFlight); 

		static Handle addRequest(const Request& newRequest);

		static void update(const int& frameInFlightIndex); 

		static StarBuffer& getBuffer(const Handle& handle); 

		static void recreate(const Handle& handle, const BufferCreationArgs& newBufferCreationArgs); 

		static void cleanup(StarDevice& device); 

		static void destroy(const Handle& handle);

	protected:
		struct CompleteRequest {
			std::unique_ptr<StarBuffer> buffer;
			std::function<void(StarBuffer&)> updateBufferData = std::function<void(StarBuffer&)>();
			std::optional<std::function<bool(void)>> checkIfNeedsUpdate = std::optional<std::function<bool(void)>>();
			std::function<void(void)> setBufferAsChanged = std::function<void(void)>();

			//1 - first frame
			//99 - done never again
			uint16_t frameInFlightIndexToUpdateOn; 

			CompleteRequest(std::unique_ptr<StarBuffer> buffer, 
				std::function<void(StarBuffer&)> updateBufferData, 
				std::function<void(void)> setBufferAsChanged,
				const uint16_t& frameInFlightIndexToUpdateOn, std::optional<std::function<bool(void)>> checkIfNeedsUpdate)
				: buffer(std::move(buffer)), updateBufferData(updateBufferData), 
				frameInFlightIndexToUpdateOn(frameInFlightIndexToUpdateOn), 
				checkIfNeedsUpdate(checkIfNeedsUpdate){};


			CompleteRequest(std::unique_ptr<StarBuffer> buffer, 
				std::function<void(StarBuffer&)> updateBufferData, 
				std::function<void(void)> setBufferAsChanged, 
				const uint16_t& frameInFlightIndexToUpdateOn)
				: buffer(std::move(buffer)), frameInFlightIndexToUpdateOn(frameInFlightIndexToUpdateOn),
				updateBufferData(updateBufferData) {};
		};

		static StarDevice* device;
		static int bufferCounter; 
		static int numFramesInFlight; 
		static int currentFrameInFlight; 
		static int staticBufferIDCounter; 
		static int dynamicBufferIDCounter; 

		static std::set<Handle> oneTimeWriteBuffersNeedWritten; 

		static std::vector<std::unique_ptr<CompleteRequest>> allBuffers; 

		//odd handles
		static std::unordered_map<Handle, std::unique_ptr<CompleteRequest>*, HandleHash> updateableBuffers;
		//even handles
		static std::unordered_map<Handle, std::unique_ptr<CompleteRequest>*, HandleHash> staticBuffers; 

		static bool isBufferStatic(const Handle& handle); 
	};
}