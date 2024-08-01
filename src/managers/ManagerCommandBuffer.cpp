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

	recordBuffers(frameIndexToBeDrawn);

	submitCommandBuffers(frameIndexToBeDrawn);
}

void star::ManagerCommandBuffer::handleNewRequests()
{
	while (!ManagerCommandBuffer::newCommandBufferRequests.empty()) {
		std::function<CommandBufferRequest(void)> requestFunction = ManagerCommandBuffer::newCommandBufferRequests.top();
		auto request = requestFunction(); 

		Handle newBufferHandle = Handle((uint32_t)this->allBuffers.size(), Handle_Type::buffer); 
		
		this->allBuffers.push_back(CompleteRequest(request.recordBufferCallback, 
			std::make_unique<StarCommandBuffer>(this->device, this->numFramesInFlight, request.type, true), 
			request.type, 
			request.recordOnce, 
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

	std::vector<CompleteRequest> buffers; 
	{
		CompleteRequest* graphicsBuffer = nullptr; 

		//only do the graphics buffer right now
		for (auto& buffer : this->allBuffers) {
			if (buffer.type == Command_Buffer_Type::Tgraphics)
				graphicsBuffer = &buffer;
		}

		assert(graphicsBuffer != nullptr && "No main graphics buffer found?");
		if (graphicsBuffer->beforeBufferSubmissionCallback.has_value())
			graphicsBuffer->beforeBufferSubmissionCallback.value()(swapChainIndex);

		if (!graphicsBuffer->recordOnce) {
			graphicsBuffer->commandBuffer->begin(swapChainIndex);
			graphicsBuffer->recordBufferCallback(graphicsBuffer->commandBuffer->buffer(swapChainIndex), swapChainIndex);
			graphicsBuffer->commandBuffer->buffer(swapChainIndex).end();
		}

		if (graphicsBuffer->overrideBufferSubmissionCallback.has_value())
			graphicsBuffer->overrideBufferSubmissionCallback.value()(*graphicsBuffer->commandBuffer, swapChainIndex);
		else
			graphicsBuffer->commandBuffer->submit(swapChainIndex);

		if (graphicsBuffer->afterBufferSubmissionCallback.has_value())
			graphicsBuffer->afterBufferSubmissionCallback.value()(swapChainIndex);
	}
	
}

void star::ManagerCommandBuffer::recordBuffers(const int& swapChainIndex)
{
	for (const auto& buffer : this->allBuffers) {

	}
}