#pragma once

#include "ManagerStorageContainer.hpp"
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
			std::unique_ptr<SharedFence> workingFence;
			std::unique_ptr<StarBuffer> buffer = nullptr; 
			boost::atomic<bool> cpuWorkDoneByTransferThread = true; 

			FinalizedBufferRequest(std::unique_ptr<BufferManagerRequest> request, std::unique_ptr<SharedFence> workingFence) : request(std::move(request)), workingFence(std::move(workingFence)){}
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

		static void destroy(const Handle& handle);

		static void cleanup(StarDevice& device); 

	protected:
		static TransferWorker* managerWorker;
		static StarDevice* managerDevice;
		static int bufferCounter; 
		static int staticBufferIDCounter; 
		static int dynamicBufferIDCounter; 

		static std::unique_ptr<ManagerStorageContainer<FinalizedBufferRequest>> bufferStorage;

		static std::set<SharedFence*> highPriorityRequestCompleteFlags;

		static void waitForFences(std::vector<vk::Fence>& fence);
	};
}
