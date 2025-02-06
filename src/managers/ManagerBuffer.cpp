#include "ManagerBuffer.hpp"

std::set<star::Handle> star::ManagerBuffer::oneTimeWriteBuffersNeedWritten = std::set<star::Handle>();
std::vector<std::unique_ptr<star::ManagerBuffer::CompleteRequest>> star::ManagerBuffer::allBuffers = std::vector<std::unique_ptr<star::ManagerBuffer::CompleteRequest>>(100);
std::unordered_map<star::Handle, std::unique_ptr<star::ManagerBuffer::CompleteRequest>*, star::HandleHash> star::ManagerBuffer::updateableBuffers = std::unordered_map<star::Handle, std::unique_ptr<star::ManagerBuffer::CompleteRequest>*, star::HandleHash>();
std::unordered_map<star::Handle, std::unique_ptr<star::ManagerBuffer::CompleteRequest>*, star::HandleHash> star::ManagerBuffer::staticBuffers = std::unordered_map<star::Handle, std::unique_ptr<star::ManagerBuffer::CompleteRequest>*, star::HandleHash>();
int star::ManagerBuffer::bufferCounter = 0; 

star::StarDevice* star::ManagerBuffer::device = nullptr; 
int star::ManagerBuffer::numFramesInFlight = 0; 
int star::ManagerBuffer::currentFrameInFlight = 0; 
int star::ManagerBuffer::staticBufferIDCounter = 0;
int star::ManagerBuffer::dynamicBufferIDCounter = 1; 

void star::ManagerBuffer::init(StarDevice& device, const int& totalNumFramesInFlight)
{
	ManagerBuffer::device = &device; 
	ManagerBuffer::numFramesInFlight = totalNumFramesInFlight; 
}

star::Handle star::ManagerBuffer::addRequest(const star::ManagerBuffer::Request& newRequest) {
	Handle newBufferHandle; 

	if (newRequest.frameInFlightIndexToUpdateOn == -1) {
		int bufferID = staticBufferIDCounter; 
		staticBufferIDCounter += 2; 

		newBufferHandle = Handle(static_cast<int>(bufferID), star::Handle_Type::buffer);
		allBuffers.at(bufferCounter) = (std::make_unique<CompleteRequest>(
			std::make_unique<StarBuffer>(
				*ManagerBuffer::device,
				newRequest.bufferCreateArgs.bufferSize,
				newRequest.bufferCreateArgs.instanceCount,
				newRequest.bufferCreateArgs.creationFlags,
				newRequest.bufferCreateArgs.memoryUsageFlags,
				newRequest.bufferCreateArgs.useFlags,
				newRequest.bufferCreateArgs.sharingMode,
				newRequest.bufferCreateArgs.minOffsetAlignment
			),
			newRequest.updateBufferData,
			newRequest.setBufferAsChanged,
			0
		)); 

		staticBuffers.emplace(std::make_pair(newBufferHandle, &allBuffers.at(bufferCounter))); 

		oneTimeWriteBuffersNeedWritten.insert(newBufferHandle); 
	}
	else {
		int bufferID = dynamicBufferIDCounter; 
		dynamicBufferIDCounter += 2; 

		newBufferHandle = Handle(static_cast<int>(bufferID), star::Handle_Type::buffer);

		allBuffers.at(bufferCounter) = std::make_unique<CompleteRequest>(
			std::make_unique<StarBuffer>(
				*ManagerBuffer::device,
				newRequest.bufferCreateArgs.bufferSize,
				newRequest.bufferCreateArgs.instanceCount,
				newRequest.bufferCreateArgs.creationFlags,
				newRequest.bufferCreateArgs.memoryUsageFlags,
				newRequest.bufferCreateArgs.useFlags,
				newRequest.bufferCreateArgs.sharingMode,
				newRequest.bufferCreateArgs.minOffsetAlignment
			),
			newRequest.updateBufferData,
			newRequest.setBufferAsChanged,
			newRequest.frameInFlightIndexToUpdateOn,
			newRequest.checkIfNeedsUpdate); 

		updateableBuffers.insert(std::make_pair(newBufferHandle, &allBuffers.at(bufferCounter))); 
	}

	bufferCounter++; 
	return newBufferHandle; 
}

void star::ManagerBuffer::update(const int& frameInFlightIndex) {
	assert(ManagerBuffer::device != nullptr && "Buffer Manager MUST be initialized first!"); 

	ManagerBuffer::currentFrameInFlight = frameInFlightIndex;

	for (auto& request : oneTimeWriteBuffersNeedWritten) {
		auto* map = isBufferStatic(request) ? &staticBuffers : &updateableBuffers;
		map->at(request)->get()->updateBufferData(*map->at(request)->get()->buffer);
	}
	oneTimeWriteBuffersNeedWritten.clear(); 

	for (auto& requestInfo : ManagerBuffer::updateableBuffers) {
		if (requestInfo.second != nullptr && requestInfo.second->get()->frameInFlightIndexToUpdateOn == static_cast<uint16_t>(frameInFlightIndex) && requestInfo.second->get()->checkIfNeedsUpdate.value()) {
			requestInfo.second->get()->updateBufferData(*requestInfo.second->get()->buffer);
		}
	}
}

star::StarBuffer& star::ManagerBuffer::getBuffer(const star::Handle& handle)
{
	assert(handle.type == star::Handle_Type::buffer && "Handle provided is not a buffer handle");

	auto* map = isBufferStatic(handle) ? &staticBuffers : &updateableBuffers;

	return *map->at(handle)->get()->buffer;
}

void star::ManagerBuffer::recreate(const star::Handle& handle, const star::ManagerBuffer::BufferCreationArgs& newBufferCreationArgs) {
	auto* map = isBufferStatic(handle) ? &staticBuffers : &updateableBuffers;

	map->at(handle)->get()->setBufferAsChanged(); 

	map->at(handle)->get()->buffer = std::make_unique<StarBuffer>(
		*ManagerBuffer::device,
		newBufferCreationArgs.bufferSize,
		newBufferCreationArgs.instanceCount,
		newBufferCreationArgs.creationFlags,
		newBufferCreationArgs.memoryUsageFlags,
		newBufferCreationArgs.useFlags,
		newBufferCreationArgs.sharingMode,
		newBufferCreationArgs.minOffsetAlignment
	);
}

void star::ManagerBuffer::cleanup(StarDevice& device)
{
	for (auto& buffer : allBuffers)
		buffer.reset(); 
}

void star::ManagerBuffer::destroy(const star::Handle& handle) {
	if (isBufferStatic(handle))
	{
		//remove the buffer from buffer storage
		staticBuffers.at(handle)->reset();

		staticBuffers.erase(handle);
		if (oneTimeWriteBuffersNeedWritten.find(handle) != oneTimeWriteBuffersNeedWritten.end())
			oneTimeWriteBuffersNeedWritten.erase(handle);
	}
	else {
		updateableBuffers.at(handle)->reset(); 
		updateableBuffers.erase(handle);
	}
}

bool star::ManagerBuffer::isBufferStatic(const star::Handle& handle)
{
	return handle.id % 2 == 0;
}