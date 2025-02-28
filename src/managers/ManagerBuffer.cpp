#include "ManagerBuffer.hpp"

std::vector<std::unique_ptr<star::ManagerBuffer::FinalizedBufferRequest>> star::ManagerBuffer::allBuffers = std::vector<std::unique_ptr<star::ManagerBuffer::FinalizedBufferRequest>>(100);
std::unordered_map<star::Handle, std::unique_ptr<star::ManagerBuffer::FinalizedBufferRequest>*, star::HandleHash> star::ManagerBuffer::updateableBuffers = std::unordered_map<star::Handle, std::unique_ptr<star::ManagerBuffer::FinalizedBufferRequest>*, star::HandleHash>();
std::unordered_map<star::Handle, std::unique_ptr<star::ManagerBuffer::FinalizedBufferRequest>*, star::HandleHash> star::ManagerBuffer::staticBuffers = std::unordered_map<star::Handle, std::unique_ptr<star::ManagerBuffer::FinalizedBufferRequest>*, star::HandleHash>();
std::stack<star::ManagerBuffer::FinalizedBufferRequest*> star::ManagerBuffer::newRequests = std::stack<star::ManagerBuffer::FinalizedBufferRequest*>();

std::set<star::SharedFence*> star::ManagerBuffer::highPriorityRequestCompleteFlags = std::set<star::SharedFence*>();
star::StarDevice* star::ManagerBuffer::managerDevice = nullptr;
star::TransferWorker* star::ManagerBuffer::managerWorker = nullptr;
int star::ManagerBuffer::managerNumFramesInFlight = 0;
int star::ManagerBuffer::bufferCounter = 0; 
int star::ManagerBuffer::currentFrameInFlight = 0; 
int star::ManagerBuffer::staticBufferIDCounter = 0;
int star::ManagerBuffer::dynamicBufferIDCounter = 1; 


void star::ManagerBuffer::init(star::StarDevice& device, star::TransferWorker& worker, const int& numFramesInFlight){
	assert(managerDevice == nullptr && "Init function should only be called once");

	managerDevice = &device;
	managerWorker = &worker;
	managerNumFramesInFlight = numFramesInFlight;
}

star::Handle star::ManagerBuffer::addRequest(std::unique_ptr<star::BufferManagerRequest> newRequest, const bool& isHighPriority) {
	Handle newBufferHandle = Handle(); 

	allBuffers.at(bufferCounter) = std::make_unique<FinalizedBufferRequest>(std::move(newRequest));

	if (!allBuffers.at(bufferCounter)->request->getFrameInFlightIndexToUpdateOn().has_value()){
		newBufferHandle = Handle(staticBufferIDCounter, star::Handle_Type::buffer); 

		staticBuffers.emplace(std::make_pair(newBufferHandle, &allBuffers.at(bufferCounter)));
			//add the new request to the transfer manager
		{
			auto* container = getRequestContainer(newBufferHandle);
			container->get()->workingFence = std::make_unique<SharedFence>(*managerDevice, true);
			container->get()->cpuWorkDoneByTransferThread.store(false);
			managerWorker->add(container->get()->request->createTransferRequest(), *container->get()->workingFence, container->get()->buffer, container->get()->cpuWorkDoneByTransferThread, false); 
			container->get()->request.release();
		}

		staticBufferIDCounter += 2;
	}else{
		newBufferHandle = Handle(dynamicBufferIDCounter, star::Handle_Type::buffer); 

		updateableBuffers.emplace(std::make_pair(newBufferHandle, &allBuffers.at(bufferCounter)));
		{
			auto* container = getRequestContainer(newBufferHandle);
			container->get()->workingFence = std::make_unique<SharedFence>(*managerDevice, true);
		}

		dynamicBufferIDCounter += 2; 
	}
	

	bufferCounter++; 
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
		for (auto& request : updateableBuffers) {
			std::unique_ptr<FinalizedBufferRequest>* container = getRequestContainer(request.first);

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
	auto* container = getRequestContainer(handle); 

	while (!container->get()->cpuWorkDoneByTransferThread.load()){
		container->get()->cpuWorkDoneByTransferThread.wait(false);
	}

	if (!isReady(handle)){
		std::cout << "Update request submitted before previous complete. Waiting..." << std::endl;

		boost::unique_lock<boost::mutex> lock; 
		vk::Fence fence; 
		container->get()->workingFence->giveMeFence(lock, fence);
		auto wait = std::vector<vk::Fence>{fence};
		waitForFences(wait); 
	}
	
	container->get()->request = std::move(newRequest); 

	managerWorker->add(container->get()->request->createTransferRequest(), *container->get()->workingFence, container->get()->buffer, container->get()->cpuWorkDoneByTransferThread, isHighPriority);

	highPriorityRequestCompleteFlags.insert(container->get()->workingFence.get());
}

bool star::ManagerBuffer::isReady(const star::Handle& handle){
	//check the fence for the buffer request
	auto* container = getRequestContainer(handle); 

	if (!container->get()->cpuWorkDoneByTransferThread.load())
	{
		boost::unique_lock<boost::mutex> lock; 
		vk::Fence fence; 
		container->get()->workingFence->giveMeFence(lock, fence);
		auto fenceResult = managerDevice->getDevice().getFenceStatus(fence);

		//if it is ready and the request has not been destroyed, can remove safely becuase the secondary thread should be done with it
		if (fenceResult == vk::Result::eSuccess){
			return true; 
		}
	}

	return false; 
}

void star::ManagerBuffer::waitForReady(const Handle& handle){
	auto* container = getRequestContainer(handle);

	while (!container->get()->cpuWorkDoneByTransferThread.load()){
		container->get()->cpuWorkDoneByTransferThread.wait(false);
	}

	{
		boost::unique_lock<boost::mutex> lock;
		vk::Fence fence; 
		container->get()->workingFence->giveMeFence(lock, fence);
		auto wait = std::vector<vk::Fence>{fence};
		waitForFences(wait); 
	}
}

star::StarBuffer& star::ManagerBuffer::getBuffer(const star::Handle& handle){
	assert(handle.getType() == star::Handle_Type::buffer && "Handle provided is not a buffer handle");

	auto* container = getRequestContainer(handle)->get(); 
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

void star::ManagerBuffer::cleanup(StarDevice& device){
	for (auto& container : allBuffers){
		if (container){
			container.reset(); 
		}
	}
}

void star::ManagerBuffer::destroy(const star::Handle& handle) {
	auto* container = getRequestContainer(handle);
	container->reset(); 
}

bool star::ManagerBuffer::isBufferStatic(const star::Handle& handle){
	return handle.getID() % 2 == 0;
}

std::unique_ptr<star::ManagerBuffer::FinalizedBufferRequest>* star::ManagerBuffer::getRequestContainer(const Handle& handle){
	assert(handle.getID() < allBuffers.size() && "Requested handle is outside range of conatiners. So it is invalid.");

	auto* map = isBufferStatic(handle) ? &staticBuffers : &updateableBuffers;

	std::unique_ptr<FinalizedBufferRequest>* container = map->at(handle);
	assert(container && "Requested container is empty");

	return container; 
}

void star::ManagerBuffer::waitForFences(std::vector<vk::Fence>& fences){

	auto result = managerDevice->getDevice().waitForFences(fences, VK_TRUE, UINT64_MAX);

	if (result != vk::Result::eSuccess)
		throw std::runtime_error("Failed to wait for fence");
}