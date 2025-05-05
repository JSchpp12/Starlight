#include "ManagerRenderResource.hpp"

std::set<star::SharedFence*> star::ManagerRenderResource::highPriorityRequestCompleteFlags = std::set<star::SharedFence*>();
std::unique_ptr<star::ManagerStorageContainer<star::ManagerRenderResource::FinalizedRenderRequest>> star::ManagerRenderResource::bufferStorage = std::make_unique<star::ManagerStorageContainer<star::ManagerRenderResource::FinalizedRenderRequest>>();

void star::ManagerRenderResource::init(star::StarDevice& device, star::TransferWorker& worker, const int& numFramesInFlight){
	assert(managerDevice == nullptr && "Init function should only be called once");

	managerDevice = &device;
	managerWorker = &worker;
}

star::Handle star::ManagerRenderResource::addRequest(std::unique_ptr<star::ManagerController::RenderResource::Buffer> newRequest, const bool& isHighPriority) {
	Handle newBufferHandle = Handle(Handle_Type::buffer); 

	bool isStatic = !newRequest->getFrameInFlightIndexToUpdateOn().has_value();

	auto requests = newRequest->createTransferRequests(managerDevice->getPhysicalDevice()); 
	auto newFull = std::make_unique<FinalizedRenderRequest>(requests.size(), std::move(newRequest), std::make_unique<SharedFence>(*managerDevice, true));
	if (isStatic){
		newFull->cpuWorkDoneByTransferThread.store(false);
		{
			auto requests = newFull->bufferRequest->createTransferRequests(managerDevice->getPhysicalDevice()); 
			for (int i = 0; i < requests.size(); i++){
				managerWorker->add(*newFull->workingFence, newFull->cpuWorkDoneByTransferThread, std::move(requests[i]), newFull->buffers[i], isHighPriority);
			}
		}

		newFull->bufferRequest.release(); 
	}

	if (isHighPriority)
		highPriorityRequestCompleteFlags.insert(newFull->workingFence.get()); 

	bufferStorage->add(std::move(newFull), isStatic, newBufferHandle); 

	return newBufferHandle; 
}

star::Handle star::ManagerRenderResource::addRequest(std::unique_ptr<star::ManagerController::RenderResource::Texture> newRequest, const bool& isHighPriority){
	Handle newHandle = Handle(Handle_Type::texture);

	bool isStatic = !newRequest->getFrameInFlightIndexToUpdateOn().has_value();
	
	auto requests = newRequest->createTransferRequests(managerDevice->getPhysicalDevice());

	auto newFull = std::make_unique<FinalizedRenderRequest>(requests.size(), std::move(newRequest), std::make_unique<SharedFence>(*managerDevice, true));
	if (isStatic){
		newFull->cpuWorkDoneByTransferThread.store(false);
		
		for (int i = 0; i < requests.size(); i++){
			managerWorker->add(*newFull->workingFence, newFull->cpuWorkDoneByTransferThread, std::move(requests[i]), newFull->textures[i], isHighPriority);
		}

		newFull->textureRequest.release();
	}

	if (isHighPriority)
		highPriorityRequestCompleteFlags.insert(newFull->workingFence.get());

	bufferStorage->add(std::move(newFull), isStatic, newHandle);

	return newHandle;
}

void star::ManagerRenderResource::update(const int& frameInFlightIndex){
	//must wait for all high priority requests to complete

	//need to make sure any previous transfers have completed before submitting
	std::vector<FinalizedRenderRequest*> requestsToUpdate = std::vector<FinalizedRenderRequest*>();
	{
		std::vector<vk::Fence> waits;
		std::vector<std::unique_ptr<boost::unique_lock<boost::mutex>>> locks = std::vector<std::unique_ptr<boost::unique_lock<boost::mutex>>>(); 
		
		//check if the request is still in processing by GPU -- wait if it is
		for (auto& request : bufferStorage->getDynamicMap()) {
			std::unique_ptr<FinalizedRenderRequest>* container = request.second;

			//check the requests which need updating this frame
			if ((container->get()->bufferRequest && !container->get()->bufferRequest->isValid(frameInFlightIndex)) || (container->get()->textureRequest && !container->get()->textureRequest->isValid(frameInFlightIndex))) {
				if (!container->get()->cpuWorkDoneByTransferThread.load())
					container->get()->cpuWorkDoneByTransferThread.wait(false);

				std::unique_ptr<boost::unique_lock<boost::mutex>> lock = std::unique_ptr<boost::unique_lock<boost::mutex>>(new boost::unique_lock<boost::mutex>());
				vk::Fence* fence = nullptr; 
				container->get()->workingFence->giveMeResource(*lock, fence);
				waits.push_back(*fence);
				locks.push_back(std::move(lock));
				
				requestsToUpdate.push_back(container->get());
			}
		}

		if (waits.size() > 0){
			waitForFences(waits);
		}
	}

	for (int i = 0; i < requestsToUpdate.size(); i++){
		requestsToUpdate[i]->cpuWorkDoneByTransferThread.store(false);
		if (requestsToUpdate[i]->bufferRequest){
			auto requests = requestsToUpdate[i]->bufferRequest->createTransferRequests(managerDevice->getPhysicalDevice());
			for (int j = 0; j < requests.size(); j++){
				managerWorker->add(*requestsToUpdate[i]->workingFence, requestsToUpdate[i]->cpuWorkDoneByTransferThread, std::move(requests[j]), requestsToUpdate[i]->buffers[j], true);
			}
		}
		else if (requestsToUpdate[i]->textureRequest){
			auto requests =  requestsToUpdate[i]->textureRequest->createTransferRequests(managerDevice->getPhysicalDevice());
			for (int j = 0; j < requests.size(); j++){
				managerWorker->add(*requestsToUpdate[i]->workingFence, requestsToUpdate[i]->cpuWorkDoneByTransferThread, std::move(requests[j]), requestsToUpdate[i]->textures[j], true);
			}
		}
	}
}

void star::ManagerRenderResource::updateRequest(std::unique_ptr<ManagerController::RenderResource::Buffer> newRequest, const star::Handle& handle, const bool& isHighPriority){
	//possible race condition....need to make sure the request on the secondary thread has been finished first before replacing
	std::unique_ptr<FinalizedRenderRequest>& container = bufferStorage->get(handle);

	while (!container->cpuWorkDoneByTransferThread.load()){
		container->cpuWorkDoneByTransferThread.wait(false);
	}

	if (!isReady(handle)){
		std::cout << "Update request submitted before previous complete. Waiting..." << std::endl;

		boost::unique_lock<boost::mutex> lock; 
		vk::Fence* fence = nullptr; 
		container->workingFence->giveMeResource(lock, fence);
		auto wait = std::vector<vk::Fence>{*fence};
		waitForFences(wait); 
	}
	
	container->bufferRequest = std::move(newRequest); 

	auto requests = container->bufferRequest->createTransferRequests(managerDevice->getPhysicalDevice());
	for (int i = 0; i < requests.size(); i++){
		managerWorker->add(*container->workingFence, container->cpuWorkDoneByTransferThread, std::move(requests[i]), container->buffers[i], isHighPriority);
	}

	highPriorityRequestCompleteFlags.insert(container->workingFence.get());
}

bool star::ManagerRenderResource::isReady(const star::Handle& handle){
	//check the fence for the buffer request
	std::unique_ptr<FinalizedRenderRequest>& container = bufferStorage->get(handle);

	if (container->cpuWorkDoneByTransferThread.load())
	{
		boost::unique_lock<boost::mutex> lock; 
		vk::Fence* fence = nullptr; 
		container->workingFence->giveMeResource(lock, fence);
		auto fenceResult = managerDevice->getDevice().getFenceStatus(*fence);

		//if it is ready and the request has not been destroyed, can remove safely becuase the secondary thread should be done with it
		if (fenceResult == vk::Result::eSuccess){
			return true; 
		}
	}

	return false; 
}

void star::ManagerRenderResource::waitForReady(const Handle& handle){
	std::unique_ptr<FinalizedRenderRequest>& container = bufferStorage->get(handle); 


	while (!container->cpuWorkDoneByTransferThread.load()){
		container->cpuWorkDoneByTransferThread.wait(false);
	}

	{
		boost::unique_lock<boost::mutex> lock;
		vk::Fence* fence = nullptr; 
		container->workingFence->giveMeResource(lock, fence);
		auto wait = std::vector<vk::Fence>{*fence};
		waitForFences(wait); 
	}
}

star::StarBuffer& star::ManagerRenderResource::getBuffer(const star::Handle& handle, const size_t& requestIndex){
	assert(handle.getType() == star::Handle_Type::buffer && "Handle provided is not a buffer handle");

	std::unique_ptr<FinalizedRenderRequest>& container = bufferStorage->get(handle);
	{
		boost::unique_lock<boost::mutex> lock;
		vk::Fence* fence = nullptr;
		container->workingFence->giveMeResource(lock, fence);

		if (managerDevice->getDevice().getFenceStatus(*fence) != vk::Result::eSuccess){
			throw std::runtime_error("Requester must call the isReady function to make sure the resource is ready before requesting it.");
		}
	}
	
	assert(requestIndex < container->buffers.size() && "Resource instance request does not exist");

	return *container->buffers[requestIndex];
}

star::StarTexture& star::ManagerRenderResource::getTexture(const star::Handle& handle, const size_t& requestIndex){
	assert(handle.getType() == star::Handle_Type::texture && "Handle provided is not a texture handle");

	std::unique_ptr<FinalizedRenderRequest>& container = bufferStorage->get(handle);
	{
		boost::unique_lock<boost::mutex> lock;
		vk::Fence* fence = nullptr; 
		container->workingFence->giveMeResource(lock, fence);

		if (managerDevice->getDevice().getFenceStatus(*fence) != vk::Result::eSuccess){
			throw std::runtime_error("Requester must call the isReady function to make sure the resource is ready before requesting it.");
		}
	}

	assert(requestIndex < container->textures.size() && "Beyond size of objects created by controller");

	return *container->textures[requestIndex];
}

void star::ManagerRenderResource::destroy(const star::Handle& handle) {
	bufferStorage->destroy(handle); 
}

void star::ManagerRenderResource::cleanup(StarDevice& device){
	bufferStorage.reset();
}