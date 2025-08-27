#pragma once

#include "Handle.hpp"
#include "device/StarDevice.hpp"
#include "TransferWorker.hpp"
#include "ManagerStorageContainer.hpp"

#include <boost/atomic.hpp>

namespace star {

class StarManager {
public:
	struct FinalizedRequest{
		boost::atomic<bool> cpuWorkDoneByTransferThread = true; 

		FinalizedRequest(){}
	};

	StarManager() = default;

	static void init(core::device::StarDevice& device, job::TransferWorker& transferWorker){
		assert(managerDevice == nullptr && "Init function should only be called once");
		managerDevice = &device; 
		managerWorker = &transferWorker;
	}

protected:
	static core::device::StarDevice* managerDevice; 
	static job::TransferWorker* managerWorker;

	static std::unique_ptr<ManagerStorageContainer<FinalizedRequest>> resourceStorage; 
	
	static void waitForFences(std::vector<vk::Fence>& fence){
		auto result = managerDevice->getVulkanDevice().waitForFences(fence, VK_TRUE, UINT64_MAX);

		if (result != vk::Result::eSuccess)
			throw std::runtime_error("Failed to wait for fence");
	}
};
}