#pragma once

#include "Handle.hpp"
#include "device/StarDevice.hpp"
#include "TransferWorker.hpp"
#include "ManagerStorageContainer.hpp"

#include <boost/atomic.hpp>

#include <memory>

namespace star {

class StarManager {
public:
	struct FinalizedRequest{
		boost::atomic<bool> cpuWorkDoneByTransferThread = true; 

		FinalizedRequest(){}
	};

	StarManager() = default;

	static void init(job::TransferWorker& transferWorker){
		managerWorker = &transferWorker;
	}

protected:
	static job::TransferWorker* managerWorker;

	static std::unique_ptr<ManagerStorageContainer<FinalizedRequest>> resourceStorage; 
};
}