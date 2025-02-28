#pragma once

#include "BufferMemoryTransferRequest.hpp"
#include "SharedFence.hpp"
#include "BufferManagerRequest.hpp"
#include "Enums.hpp"
#include "StarBuffer.hpp"
#include "StarDevice.hpp"
#include "StarManager.hpp"
#include "Handle.hpp"
#include "TransferWorker.hpp"

#include <vulkan/vulkan.hpp>

#include <boost/atomic.hpp>

#include <memory>
#include <map>
#include <array>
#include <queue>
#include <functional>
#include <stack>
#include <optional>
#include <set>

#include <vma/vk_mem_alloc.h>

namespace star {
	class ManagerBuffer : public StarManager{
	public:
		struct FinalizedBufferRequest {
			std::unique_ptr<BufferManagerRequest> request = nullptr;
			std::unique_ptr<StarBuffer> buffer = nullptr; 
			std::unique_ptr<SharedFence> workingFence;
			boost::atomic<bool> cpuWorkDoneByTransferThread = true; 

			FinalizedBufferRequest(std::unique_ptr<BufferManagerRequest> request) 
			: request(std::move(request)){}
		};

		static void init(StarDevice& device, TransferWorker& worker, const int& totalNumFramesInFlight); 

		static Handle addRequest(std::unique_ptr<BufferManagerRequest> newRequest, const bool& isHighPriority = false);

		/// @brief Submit request to write new data to a buffer already created and associated to a handle
		/// @param newRequest New data request
		/// @param handle Handle to resource
		static void updateRequest(std::unique_ptr<BufferManagerRequest> newRequest, const Handle& handle, const bool& isHighPriority = false); 

		static void update(const int& frameInFlightIndex); 

		static bool isReady(const Handle& handle);

		static void waitForReady(const Handle& handle);

		static void waitForAllHighPriorityRequests(); 

		static StarBuffer& getBuffer(const Handle& handle); 

		// static void recreate(const Handle& handle, const BufferManagerRequest::BufferCreationArgs& newBufferCreationArgs); 

		static void cleanup(StarDevice& device); 

		static void destroy(const Handle& handle);
	protected:
		static TransferWorker* managerWorker;
		static StarDevice* managerDevice;
		static int bufferCounter; 
		static int managerNumFramesInFlight; 
		static int currentFrameInFlight; 
		static int staticBufferIDCounter; 
		static int dynamicBufferIDCounter; 

		static std::vector<std::unique_ptr<FinalizedBufferRequest>> allBuffers;

		static std::stack<FinalizedBufferRequest*> newRequests; 
		static std::set<SharedFence*> highPriorityRequestCompleteFlags;

		//odd handles
		static std::unordered_map<Handle, std::unique_ptr<FinalizedBufferRequest>*, HandleHash> updateableBuffers;
		//even handles
		static std::unordered_map<Handle, std::unique_ptr<FinalizedBufferRequest>*, HandleHash> staticBuffers; 

		// static std::unordered_set<Handle> changedBuffers;
		static bool isBufferStatic(const Handle& handle); 

		static std::unique_ptr<FinalizedBufferRequest>* getRequestContainer(const Handle& handle); 

		static void waitForFences(std::vector<vk::Fence>& fence);
	};
}
