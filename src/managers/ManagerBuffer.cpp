#include "ManagerBuffer.hpp"

std::vector<std::unique_ptr<star::ManagerBuffer::FinalizedBufferRequest>> star::ManagerBuffer::allBuffers = std::vector<std::unique_ptr<star::ManagerBuffer::FinalizedBufferRequest>>(100);
std::unordered_map<star::Handle, std::unique_ptr<star::ManagerBuffer::FinalizedBufferRequest>*, star::HandleHash> star::ManagerBuffer::updateableBuffers = std::unordered_map<star::Handle, std::unique_ptr<star::ManagerBuffer::FinalizedBufferRequest>*, star::HandleHash>();
std::unordered_map<star::Handle, std::unique_ptr<star::ManagerBuffer::FinalizedBufferRequest>*, star::HandleHash> star::ManagerBuffer::staticBuffers = std::unordered_map<star::Handle, std::unique_ptr<star::ManagerBuffer::FinalizedBufferRequest>*, star::HandleHash>();
std::stack<star::ManagerBuffer::FinalizedBufferRequest*> star::ManagerBuffer::newRequests = std::stack<star::ManagerBuffer::FinalizedBufferRequest*>();

std::set<vk::Fence> star::ManagerBuffer::highPriorityRequestCompleteFlags = std::set<vk::Fence>();
star::StarDevice* star::ManagerBuffer::managerDevice = nullptr;
star::TransferWorker* star::ManagerBuffer::managerWorker = nullptr;
int star::ManagerBuffer::managerNumFramesInFlight = 0;
int star::ManagerBuffer::bufferCounter = 0; 
int star::ManagerBuffer::currentFrameInFlight = 0; 
int star::ManagerBuffer::staticBufferIDCounter = 0;
int star::ManagerBuffer::dynamicBufferIDCounter = 1; 


void star::ManagerBuffer::init(star::StarDevice& device, star::TransferWorker& worker, const int& numFramesInFlight)
{
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

		staticBufferIDCounter += 2;
	}else{
		newBufferHandle = Handle(dynamicBufferIDCounter, star::Handle_Type::buffer); 

		updateableBuffers.emplace(std::make_pair(newBufferHandle, &allBuffers.at(bufferCounter)));

		dynamicBufferIDCounter += 2; 
	}
	
	//add the new request to the transfer manager
	{
		vk::FenceCreateInfo info{}; 
		info.sType = vk::StructureType::eFenceCreateInfo;
		info.flags = vk::FenceCreateFlagBits::eSignaled;
	
		auto* container = getRequestContainer(newBufferHandle);
		container->get()->workingFence = managerDevice->getDevice().createFence(info);
		managerWorker->add(container->get()->request->createTransferRequest(), &container->get()->workingFence, container->get()->buffer, isHighPriority);

		auto& testBuffer = getBuffer(newBufferHandle);

		std::cout << "test" << std::endl; 
	}

	bufferCounter++; 
	return newBufferHandle; 
}

void star::ManagerBuffer::update(const int& frameInFlightIndex){
	//must wait for all high priority requests to complete

	//need to make sure any previous transfers have completed before submitting
	std::vector<FinalizedBufferRequest*> requestsToUpdate = std::vector<FinalizedBufferRequest*>();
	std::vector<vk::Fence> waits;
	{
		for (auto& request : updateableBuffers) {
			std::unique_ptr<FinalizedBufferRequest>* container = getRequestContainer(request.first);

			//check the requests which need updating this frame
			if (!container->get()->request->isValid(frameInFlightIndex)) {
			//check if the request is still in processing -- wait if it i
				waits.push_back(container->get()->workingFence);
				requestsToUpdate.push_back(container->get());
			}
		}

		if (waits.size() > 0)
			waitForFences(waits);
	}

	for (int i = 0; i < requestsToUpdate.size(); i++){
		managerWorker->add(requestsToUpdate[i]->request->createTransferRequest(), &requestsToUpdate[i]->workingFence, requestsToUpdate[i]->buffer, true);
	}

	if (waits.size() > 0)
		waitForFences(waits); 
}

void star::ManagerBuffer::updateRequest(std::unique_ptr<BufferManagerRequest> newRequest, const star::Handle& handle, const bool& isHighPriority){
	//possible race condition....need to make sure the request on the secondary thread has been finished first before replacing
	auto* container = getRequestContainer(handle); 

	auto wait = std::vector<vk::Fence>{container->get()->workingFence};
	if (!isReady(handle)){
		std::cout << "Update request submitted before previous complete. Waiting..." << std::endl;

		waitForFences(wait); 
	}
	
	container->get()->request = std::move(newRequest); 

	managerWorker->add(container->get()->request->createTransferRequest(), &container->get()->workingFence, container->get()->buffer, isHighPriority);

	highPriorityRequestCompleteFlags.insert(container->get()->workingFence);
}

bool star::ManagerBuffer::isReady(const star::Handle& handle){
	//check the fence for the buffer request
	auto* conatiner = getRequestContainer(handle); 

	auto fenceResult = managerDevice->getDevice().getFenceStatus(conatiner->get()->workingFence);

	//if it is ready and the request has not been destroyed, can remove safely becuase the secondary thread should be done with it
	if (fenceResult == vk::Result::eSuccess){
		if (conatiner->get()->request)
			conatiner->get()->request.reset();
		return true; 
	}

	return false; 
}

star::StarBuffer& star::ManagerBuffer::getBuffer(const star::Handle& handle)
{
	assert(handle.getType() == star::Handle_Type::buffer && "Handle provided is not a buffer handle");

	return *getRequestContainer(handle)->get()->buffer;
}

void star::ManagerBuffer::cleanup(StarDevice& device)
{
	for (auto& buffer : allBuffers)
		buffer.reset(); 
}

void star::ManagerBuffer::destroy(const star::Handle& handle) {
	auto* container = getRequestContainer(handle);
	container->reset(); 
}

bool star::ManagerBuffer::isBufferStatic(const star::Handle& handle)
{
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