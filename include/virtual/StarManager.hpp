#pragma once

#include "Handle.hpp"
#include "StarDevice.hpp"
#include "TransferWorker.hpp"
#include "ManagerStorageContainer.hpp"

#include <boost/atomic.hpp>

namespace star {

class StarManager {
public:
	struct FinalizedRequest{
		boost::atomic<bool> cpuWorkDoneByTransferThread = true; 
		std::unique_ptr<SharedFence> workingFence = nullptr; 

		FinalizedRequest(std::unique_ptr<SharedFence> workingFence) : workingFence(std::move(workingFence)){}
	};

	StarManager() = default;

	static void init(StarDevice& device, TransferWorker& transferWorker){
		assert(managerDevice == nullptr && "Init function should only be called once");
		managerDevice = &device; 
		managerWorker = &transferWorker;
	}

	virtual void destroy(const Handle& resourceHandle) = 0; 

protected:
	static StarDevice* managerDevice; 
	static TransferWorker* managerWorker;

	static std::unique_ptr<ManagerStorageContainer<FinalizedRequest>> resourceStorage; 
	
	static void waitForFences(std::vector<vk::Fence>& fence){
		auto result = managerDevice->getDevice().waitForFences(fence, VK_TRUE, UINT64_MAX);

		if (result != vk::Result::eSuccess)
			throw std::runtime_error("Failed to wait for fence");
	}
};
}