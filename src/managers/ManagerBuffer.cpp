#include "ManagerBuffer.hpp"

std::stack<star::ManagerBuffer::CompleteRequest*> star::ManagerBuffer::oneTimeWriteBuffersNeedWritten = std::stack<star::ManagerBuffer::CompleteRequest*>();
std::unordered_map<star::Handle, star::ManagerBuffer::CompleteRequest, star::HandleHash> star::ManagerBuffer::updateableBuffers = std::unordered_map<star::Handle, star::ManagerBuffer::CompleteRequest, star::HandleHash>();
std::unordered_map<star::Handle, star::ManagerBuffer::CompleteRequest, star::HandleHash> star::ManagerBuffer::staticBuffers = std::unordered_map<star::Handle, star::ManagerBuffer::CompleteRequest, star::HandleHash>();

star::StarDevice* star::ManagerBuffer::device = nullptr; 
int star::ManagerBuffer::numFramesInFlight = 0; 
int star::ManagerBuffer::currentFrameInFlight = 0; 
int star::ManagerBuffer::bufferCounter = 0; 

void star::ManagerBuffer::init(StarDevice& device, const int& totalNumFramesInFlight)
{
	ManagerBuffer::device = &device; 
	ManagerBuffer::numFramesInFlight = totalNumFramesInFlight; 
}

star::Handle star::ManagerBuffer::addRequest(const star::ManagerBuffer::Request& newRequest) {
	int bufferID = static_cast<int>(bufferCounter); 
	bufferCounter++; 

	Handle newBufferHandle; 
	if (newRequest.frameInFlightIndexToUpdateOn == -1) {
		
		if (bufferID % 2 != 0) {
			bufferID++; 
		}

		newBufferHandle = Handle(static_cast<int>(bufferID), star::Handle_Type::buffer);
		staticBuffers.emplace(std::make_pair(newBufferHandle, CompleteRequest(
			std::make_unique<StarBuffer>(
				*ManagerBuffer::device,
				newRequest.bufferSize,
				newRequest.instanceCount,
				newRequest.creationFlags,
				newRequest.memoryUsageFlags,
				newRequest.useFlags,
				newRequest.sharingMode,
				newRequest.minOffsetAlignment
			),
			newRequest.updateBufferData, 
			0
		)));

		oneTimeWriteBuffersNeedWritten.push(&staticBuffers.at(newBufferHandle)); 
	}
	else {

		if (bufferID % 2 == 0) {
			bufferID++; 
		}

		newBufferHandle = Handle(static_cast<int>(bufferID), star::Handle_Type::buffer); 
		updateableBuffers.emplace(std::make_pair(newBufferHandle, CompleteRequest(
			std::make_unique<StarBuffer>(
				*ManagerBuffer::device,
				newRequest.bufferSize,
				newRequest.instanceCount,
				newRequest.creationFlags,
				newRequest.memoryUsageFlags,
				newRequest.useFlags,
				newRequest.sharingMode,
				newRequest.minOffsetAlignment
				),
			newRequest.updateBufferData,
			newRequest.frameInFlightIndexToUpdateOn
		)));
	}
	
	return newBufferHandle; 
}

void star::ManagerBuffer::update(const int& frameInFlightIndex) {
	assert(ManagerBuffer::device != nullptr && "Buffer Manager MUST be initialized first!"); 

	ManagerBuffer::currentFrameInFlight = frameInFlightIndex;

	while (!oneTimeWriteBuffersNeedWritten.empty()) {
		CompleteRequest* request = oneTimeWriteBuffersNeedWritten.top(); 
		request->updateBufferData(*request->buffer); 
		oneTimeWriteBuffersNeedWritten.pop(); 
	}

	for (auto& buffer : ManagerBuffer::updateableBuffers) {
		if (buffer.second.frameInFlightIndexToUpdateOn == static_cast<uint16_t>(frameInFlightIndex)) {
			buffer.second.updateBufferData(*buffer.second.buffer);
		}
	}
}

star::StarBuffer& star::ManagerBuffer::getBuffer(const star::Handle& handle)
{
	assert(handle.type == star::Handle_Type::buffer && "Handle provided is not a buffer handle");

	if (handle.id % 2 == 0)
		return *ManagerBuffer::staticBuffers.at(handle).buffer;

	return *ManagerBuffer::updateableBuffers.at(handle).buffer;
}

void star::ManagerBuffer::cleanup(StarDevice& device)
{
	for (auto& request : ManagerBuffer::updateableBuffers) {
		updateableBuffers.erase(request.first);
	}

	for (auto& request : staticBuffers) {
		staticBuffers.erase(request.first);
	}
}

void star::ManagerBuffer::destroy(const star::Handle& handle) {
	std::unordered_map<Handle, CompleteRequest, HandleHash>* map = getMapForBuffer(handle);
	map->erase(handle); 
}

std::unordered_map<star::Handle, star::ManagerBuffer::CompleteRequest, star::HandleHash>* star::ManagerBuffer::getMapForBuffer(const star::Handle& handle)
{
	if (handle.id % 2 == 0)
		return &staticBuffers;

	return &updateableBuffers;
}