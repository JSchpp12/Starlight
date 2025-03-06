#include "ManagerBuffer.hpp"

std::set<star::SharedFence*> star::ManagerBuffer::highPriorityRequestCompleteFlags = std::set<star::SharedFence*>();
star::StarDevice* star::ManagerBuffer::managerDevice = nullptr;
star::TransferWorker* star::ManagerBuffer::managerWorker = nullptr;
std::unique_ptr<star::ManagerStorageContainer<star::ManagerBuffer::FinalizedBufferRequest>> star::ManagerBuffer::bufferStorage = std::make_unique<star::ManagerStorageContainer<star::ManagerBuffer::FinalizedBufferRequest>>();


void star::ManagerBuffer::init(star::StarDevice& device, star::TransferWorker& worker, const int& numFramesInFlight){
	assert(managerDevice == nullptr && "Init function should only be called once");

	managerDevice = &device;
	managerWorker = &worker;
}

star::Handle star::ManagerBuffer::addRequest(std::unique_ptr<star::BufferManagerRequest> newRequest, const bool& isHighPriority) {
	Handle newBufferHandle = Handle(Handle_Type::buffer); 

	bool isStatic = !newRequest->getFrameInFlightIndexToUpdateOn().has_value();

	auto newFull = std::make_unique<FinalizedBufferRequest>(std::move(newRequest), std::make_unique<SharedFence>(*managerDevice, true));
	if (isStatic){
		newFull->cpuWorkDoneByTransferThread.store(false);
		managerWorker->add(newFull->request->createTransferRequest(), *newFull->workingFence, newFull->buffer, newFull->cpuWorkDoneByTransferThread, isHighPriority);
		newFull->request.release(); 
	}

	if (isHighPriority)
		highPriorityRequestCompleteFlags.insert(newFull->workingFence.get()); 


	bufferStorage->add(std::move(newFull), isStatic, newBufferHandle); 

	return newBufferHandle; 
}

void star::ManagerBuffer::update(const int& frameInFlightIndex){
	//must wait for all high priority requests to complete

	//need to make sure any previous transfers have completed before submitting
	std::vector<FinalizedBufferRequest*> requestsToUpdate = std::vector<FinalizedBufferRequest*>();
	{
		std::vector<vk::Fence> waits;
		std::vector<std::unique_ptr<boost::unique_lock<boost::mutex>>> locks = std::vector<std::unique_ptr<boost::unique_lock<boost::mutex>>>(); 
		
		//check if the request is still in processing by GPU -- wait if it is
		for (auto& request : bufferStorage->getDynamicMap()) {
			std::unique_ptr<FinalizedBufferRequest>* container = request.second;

			//check the requests which need updating this frame
			if (!container->get()->request->isValid(frameInFlightIndex)) {
				if (!container->get()->cpuWorkDoneByTransferThread.load())
					container->get()->cpuWorkDoneByTransferThread.wait(false);

				std::unique_ptr<boost::unique_lock<boost::mutex>> lock = std::unique_ptr<boost::unique_lock<boost::mutex>>(new boost::unique_lock<boost::mutex>());
				vk::Fence fence; 
				container->get()->workingFence->giveMeFence(*lock, fence);
				waits.push_back(fence);
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
		managerWorker->add(requestsToUpdate[i]->request->createTransferRequest(), *requestsToUpdate[i]->workingFence, requestsToUpdate[i]->buffer, requestsToUpdate[i]->cpuWorkDoneByTransferThread, true);
	}
}

void star::ManagerBuffer::updateRequest(std::unique_ptr<BufferManagerRequest> newRequest, const star::Handle& handle, const bool& isHighPriority){
	//possible race condition....need to make sure the request on the secondary thread has been finished first before replacing
	std::unique_ptr<FinalizedBufferRequest>& container = bufferStorage->get(handle);

	while (!container->cpuWorkDoneByTransferThread.load()){
		container->cpuWorkDoneByTransferThread.wait(false);
	}

	if (!isReady(handle)){
		std::cout << "Update request submitted before previous complete. Waiting..." << std::endl;

		boost::unique_lock<boost::mutex> lock; 
		vk::Fence fence; 
		container->workingFence->giveMeFence(lock, fence);
		auto wait = std::vector<vk::Fence>{fence};
		waitForFences(wait); 
	}
	
	container->request = std::move(newRequest); 

	managerWorker->add(container->request->createTransferRequest(), *container->workingFence, container->buffer, container->cpuWorkDoneByTransferThread, isHighPriority);

	highPriorityRequestCompleteFlags.insert(container->workingFence.get());
}

bool star::ManagerBuffer::isReady(const star::Handle& handle){
	//check the fence for the buffer request
	std::unique_ptr<FinalizedBufferRequest>& container = bufferStorage->get(handle);


	if (!container->cpuWorkDoneByTransferThread.load())
	{
		boost::unique_lock<boost::mutex> lock; 
		vk::Fence fence; 
		container->workingFence->giveMeFence(lock, fence);
		auto fenceResult = managerDevice->getDevice().getFenceStatus(fence);

		//if it is ready and the request has not been destroyed, can remove safely becuase the secondary thread should be done with it
		if (fenceResult == vk::Result::eSuccess){
			return true; 
		}
	}

	return false; 
}

void star::ManagerBuffer::waitForReady(const Handle& handle){
	std::unique_ptr<FinalizedBufferRequest>& container = bufferStorage->get(handle); 


	while (!container->cpuWorkDoneByTransferThread.load()){
		container->cpuWorkDoneByTransferThread.wait(false);
	}

	{
		boost::unique_lock<boost::mutex> lock;
		vk::Fence fence; 
		container->workingFence->giveMeFence(lock, fence);
		auto wait = std::vector<vk::Fence>{fence};
		waitForFences(wait); 
	}
}

star::StarBuffer& star::ManagerBuffer::getBuffer(const star::Handle& handle){
	assert(handle.getType() == star::Handle_Type::buffer && "Handle provided is not a buffer handle");

	std::unique_ptr<FinalizedBufferRequest>& container = bufferStorage->get(handle);
	{
		boost::unique_lock<boost::mutex> lock;
		vk::Fence fence;
		container->workingFence->giveMeFence(lock, fence);

		if (managerDevice->getDevice().getFenceStatus(fence) != vk::Result::eSuccess){
			throw std::runtime_error("Requester must call the isReady function to make sure the resource is ready before requesting it.");
		}
	}

	return *container->buffer;
}

void star::ManagerBuffer::destroy(const star::Handle& handle) {
	bufferStorage->destroy(handle); 
}

void star::ManagerBuffer::cleanup(StarDevice& device){
	bufferStorage.release();
}

void star::ManagerBuffer::waitForFences(std::vector<vk::Fence>& fences){

	auto result = managerDevice->getDevice().waitForFences(fences, VK_TRUE, UINT64_MAX);

	if (result != vk::Result::eSuccess)
		throw std::runtime_error("Failed to wait for fence");
}