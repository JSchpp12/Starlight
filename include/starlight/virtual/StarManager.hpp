#pragma once

#include "device/StarDevice.hpp"
#include "job/TransferWorker.hpp"
#include "ManagerStorageContainer.hpp"

#include <star_common/Handle.hpp>

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

	static void init(std::shared_ptr<job::TransferWorker> transferWorker){
		managerWorker = transferWorker;
	}

protected:
	static std::shared_ptr<job::TransferWorker> managerWorker;

	static std::unique_ptr<ManagerStorageContainer<FinalizedRequest>> resourceStorage; 
};
}