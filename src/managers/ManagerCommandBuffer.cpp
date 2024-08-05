#include "ManagerCommandBuffer.hpp"

std::stack<std::function<star::ManagerCommandBuffer::CommandBufferRequest(void)>> star::ManagerCommandBuffer::newCommandBufferRequests = std::stack<std::function<star::ManagerCommandBuffer::CommandBufferRequest(void)>>();

std::stack<star::Handle> star::ManagerCommandBuffer::dynamicBuffersToSubmit = std::stack<star::Handle>();

void star::ManagerCommandBuffer::request(std::function<CommandBufferRequest(void)> request)
{
	ManagerCommandBuffer::newCommandBufferRequests.push(request);
}

void star::ManagerCommandBuffer::submitDynamicBuffer(Handle bufferHandle)
{
	ManagerCommandBuffer::dynamicBuffersToSubmit.push(bufferHandle);
}

vk::Semaphore* star::ManagerCommandBuffer::getMainGraphicsDoneSemaphore(const int& frameIndexToBeDrawn)
{
	return &this->mainGraphicsBuffer->commandBuffer->getCompleteSemaphores().at(frameIndexToBeDrawn);
}

void star::ManagerCommandBuffer::update(const int& frameIndexToBeDrawn)
{
	handleNewRequests();

	submitCommandBuffers(frameIndexToBeDrawn);
}

void star::ManagerCommandBuffer::handleNewRequests()
{
	while (!ManagerCommandBuffer::newCommandBufferRequests.empty()) {
		std::function<CommandBufferRequest(void)> requestFunction = ManagerCommandBuffer::newCommandBufferRequests.top();
		auto request = requestFunction(); 

		Handle newBufferHandle = Handle((uint32_t)this->allBuffers.size(), Handle_Type::buffer); 
		
		this->allBuffers.push_back(CompleteRequest(
			request.recordBufferCallback, 
			std::make_unique<StarCommandBuffer>(this->device, this->numFramesInFlight, request.type, true), 
			request.type, 
			request.recordOnce, 
			request.order,
			request.beforeBufferSubmissionCallback, 
			request.afterBufferSubmissionCallback, 
			request.overrideBufferSubmissionCallback));
			
		if (request.type == Command_Buffer_Type::Tgraphics)
			this->mainGraphicsBuffer = &this->allBuffers.at(newBufferHandle.id);

		if (request.willBeSubmittedEachFrame) {
			this->standardBuffers.push_back(newBufferHandle); 
		}
		else {
			this->dynamicBuffers.push_back(newBufferHandle); 
		}

		if (request.recordOnce) {
			for (int i = 0; i < this->numFramesInFlight; i++) {
				this->allBuffers.at(newBufferHandle.id).commandBuffer->begin(i);
				request.recordBufferCallback(this->allBuffers.at(newBufferHandle.id).commandBuffer->buffer(i), i);
			}
		}

		ManagerCommandBuffer::newCommandBufferRequests.pop();
	}
}

void star::ManagerCommandBuffer::submitCommandBuffers(const int& swapChainIndex)
{
	//determine the order of buffers to execute
	assert(this->mainGraphicsBuffer != nullptr && "No main graphics buffer set -- cannot happen");

	std::vector<CompleteRequest*> buffersBeforeGraphics;
	std::vector<CompleteRequest*> buffersAfterGraphics; 
	{
		CompleteRequest* finalBuffer = nullptr; 

		//only do the graphics buffer right now
		for (auto& buffer : this->allBuffers) {
			if (buffer.type != Command_Buffer_Type::Tgraphics) {
				switch (buffer.order) {
				case(CommandBufferOrder::BEOFRE_RENDER_PASS):
					buffersBeforeGraphics.push_back(&buffer);
					break;
				case(CommandBufferOrder::AFTER_RENDER_PASS):
					buffersAfterGraphics.push_back(&buffer);
					break;
				case(CommandBufferOrder::END_OF_FRAME):
					assert(finalBuffer == nullptr && "Multiple buffers assigned to end of frame -- cannot happen");
					finalBuffer = &buffer;
					break;
				default:
					throw std::runtime_error("Unknown buffer order");
				}
			}
		}

		//need to submit each group of buffers depending on the queue family they are in
		//std::array<std::vector<StarBuffer>, 3> buffers = { buffersBeforeGraphics, {this->mainthis->mainGraphicsBuffer}, buffersAfterGraphics };

		if (this->mainGraphicsBuffer->beforeBufferSubmissionCallback.has_value())
			this->mainGraphicsBuffer->beforeBufferSubmissionCallback.value()(swapChainIndex);

		if (!this->mainGraphicsBuffer->recordOnce) {
			this->mainGraphicsBuffer->commandBuffer->begin(swapChainIndex);
			this->mainGraphicsBuffer->recordBufferCallback(this->mainGraphicsBuffer->commandBuffer->buffer(swapChainIndex), swapChainIndex);
			this->mainGraphicsBuffer->commandBuffer->buffer(swapChainIndex).end();
		}

		if (this->mainGraphicsBuffer->overrideBufferSubmissionCallback.has_value())
			this->mainGraphicsBuffer->overrideBufferSubmissionCallback.value()(*this->mainGraphicsBuffer->commandBuffer, swapChainIndex);
		else
			this->mainGraphicsBuffer->commandBuffer->submit(swapChainIndex);

		if (this->mainGraphicsBuffer->afterBufferSubmissionCallback.has_value())
			this->mainGraphicsBuffer->afterBufferSubmissionCallback.value()(swapChainIndex);
	}

	//submit the buffers required before the graphics buffer

	//submit the main graphics buffer

	//submit the buffers required after the graphics buffer
	
}