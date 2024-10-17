#include "ManagerBuffer.hpp"

std::vector<std::unique_ptr<star::ManagerBuffer::CompleteRequest>> star::ManagerBuffer::allBuffers = std::vector<std::unique_ptr<star::ManagerBuffer::CompleteRequest>>(); 
star::StarDevice* star::ManagerBuffer::device = nullptr; 
int star::ManagerBuffer::numFramesInFlight = 0; 
int star::ManagerBuffer::currentFrameInFlight = 0; 

void star::ManagerBuffer::init(StarDevice& device, const int& totalNumFramesInFlight)
{
	ManagerBuffer::device = &device; 
	ManagerBuffer::numFramesInFlight = totalNumFramesInFlight; 
}

star::Handle star::ManagerBuffer::addRequest(const star::ManagerBuffer::Request& newRequest) {
	int bufferID = static_cast<int>(allBuffers.size()); 
	
	allBuffers.emplace_back(std::make_unique<CompleteRequest>(
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
	));

	return Handle(bufferID, star::Handle_Type::buffer);
}

void star::ManagerBuffer::update(const int& frameInFlightIndex) {
	assert(ManagerBuffer::device != nullptr && "Buffer Manager MUST be initialized first!"); 

	ManagerBuffer::currentFrameInFlight = frameInFlightIndex;

	for (std::unique_ptr<CompleteRequest>& buffer : ManagerBuffer::allBuffers) {
		if (buffer && buffer->frameInFlightIndexToUpdateOn == static_cast<uint16_t>(frameInFlightIndex))
			buffer->updateBufferData(*buffer->buffer); 
	}
}

star::StarBuffer& star::ManagerBuffer::getBuffer(const star::Handle& handle)
{
	assert(handle.type == star::Handle_Type::buffer && "Handle provided is not a buffer handle");

	return *ManagerBuffer::allBuffers.at(handle.id)->buffer;
}

void star::ManagerBuffer::cleanup(StarDevice& device)
{
	for (std::unique_ptr<CompleteRequest>& request : ManagerBuffer::allBuffers) {
		request.reset(); 
	}
}

void star::ManagerBuffer::destroy(const star::Handle& handle) {
	ManagerBuffer::allBuffers.at(handle.id).reset();
}