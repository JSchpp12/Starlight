#include "internals/CommandBufferContainer.hpp"

std::vector<vk::Semaphore> star::CommandBufferContainer::submitGroupWhenReady(const star::Command_Buffer_Order& order, const int& swapChainIndex, std::vector<vk::Semaphore>* waitSemaphores)
{
	std::vector<vk::Semaphore> semaphores;
	std::vector<std::pair<CompleteRequest&, vk::Fence>> buffersToSubmitWithFences = std::vector<std::pair<CompleteRequest&, vk::Fence>>();

	for (int type = star::Command_Buffer_Type::Tgraphics; type != star::Command_Buffer_Type::Tcompute; type++) {
		std::vector<std::reference_wrapper<CompleteRequest>> buffersToSubmit = this->getAllBuffersOfTypeAndOrderReadyToSubmit(order, static_cast<star::Command_Buffer_Type>(type), true);

		if (!buffersToSubmit.empty()) {
			waitUntilOrderGroupReady(swapChainIndex, order, static_cast<Command_Buffer_Type>(type));

			auto waitPoints = std::vector<vk::PipelineStageFlags>();
			for (auto& buffer : buffersToSubmit) {
				waitPoints.push_back(buffer.get().waitStage);
			}

			//before submission
			for (CompleteRequest& buffer : buffersToSubmit) {
				if (buffer.beforeBufferSubmissionCallback.has_value())
					buffer.beforeBufferSubmissionCallback.value()(swapChainIndex);

				if (!buffer.recordOnce) {
					buffer.commandBuffer->begin(swapChainIndex);
					buffer.recordBufferCallback(buffer.commandBuffer->buffer(swapChainIndex), swapChainIndex);
					buffer.commandBuffer->buffer(swapChainIndex).end();
				}
			}

			//submit
			{
				std::vector<vk::CommandBuffer> buffers = std::vector<vk::CommandBuffer>();
				for (CompleteRequest& buffer : buffersToSubmit) {
					buffers.push_back(buffer.commandBuffer->buffer(swapChainIndex));
				}

				vk::SubmitInfo submitInfo{};

				if (waitSemaphores != nullptr) {
					submitInfo.waitSemaphoreCount = waitSemaphores->size();
					submitInfo.pWaitSemaphores = waitSemaphores->data();
					submitInfo.pWaitDstStageMask = waitPoints.data();
				}

				submitInfo.signalSemaphoreCount = 1;
				submitInfo.pSignalSemaphores = &this->bufferGroups[order]->semaphores[static_cast<Command_Buffer_Type>(type)].at(swapChainIndex);

				//record for later
				semaphores.push_back(this->bufferGroups[order]->semaphores[static_cast<Command_Buffer_Type>(type)].at(swapChainIndex));

				submitInfo.pCommandBuffers = buffers.data();
				submitInfo.commandBufferCount = buffers.size();

				std::unique_ptr<vk::Result> commandResult = std::unique_ptr<vk::Result>();
				vk::Fence workingFence = this->bufferGroups[order]->fences[static_cast<Command_Buffer_Type>(type)].at(swapChainIndex);
				switch (type) {
				case(Command_Buffer_Type::Tgraphics):
					commandResult = std::make_unique<vk::Result>(this->device.getGraphicsQueue().submit(1, &submitInfo, workingFence));
					break;
				case(Command_Buffer_Type::Tcompute):
					commandResult = std::make_unique<vk::Result>(this->device.getComputeQueue().submit(1, &submitInfo, workingFence));
					break;
				case(Command_Buffer_Type::Ttransfer):
					commandResult = std::make_unique<vk::Result>(this->device.getTransferQueue().submit(1, &submitInfo, workingFence));
					break;
				default:
					break;
				}

				assert(commandResult != nullptr && "Invalid command buffer type");

				if (*commandResult.get() != vk::Result::eSuccess) {
					throw std::runtime_error("Failed to submit command buffer");
				}

				for (auto& buffer : buffersToSubmit) {
					buffersToSubmitWithFences.push_back(std::make_pair(buffer, workingFence));
				}
			}
		}
	}

	//after submit
	for (auto& bufferAndFence : buffersToSubmitWithFences) {
		if (bufferAndFence.first.afterBufferSubmissionCallback.has_value())
			bufferAndFence.first.afterBufferSubmissionCallback.value()(swapChainIndex);

		this->device.getDevice().waitForFences(bufferAndFence.second, VK_TRUE, UINT64_MAX);
	}

	return semaphores;
}

star::Handle star::CommandBufferContainer::add(std::unique_ptr<star::CommandBufferContainer::CompleteRequest> newRequest, const bool& willBeSubmittedEachFrame, const star::Command_Buffer_Type& type, const star::Command_Buffer_Order& order) {
	star::Handle newHandle = star::Handle(this->allBuffers.size(), star::Handle_Type::buffer);
	const int bufferIndex = this->allBuffers.size();

	this->allBuffers.push_back(std::move(newRequest));
	this->bufferSubmissionStatus.push_back(willBeSubmittedEachFrame ? 2 : 0);

	this->bufferGroups[order]->bufferOrderGroupsIndices[type].push_back(bufferIndex);

	return newHandle;
}

bool star::CommandBufferContainer::shouldSubmitThisBuffer(const size_t& bufferIndex)
{
	assert(bufferIndex < this->allBuffers.size() && "Requested index does not exist");

	return this->bufferSubmissionStatus[bufferIndex] & 1 || this->bufferSubmissionStatus[bufferIndex] & 2;
}

void star::CommandBufferContainer::resetThisBufferStatus(const size_t& bufferIndex)
{
	assert(bufferIndex < this->allBuffers.size() && "Requested index does not exist");

	if (this->bufferSubmissionStatus[bufferIndex] & 1)
		this->bufferSubmissionStatus[bufferIndex] = 0;
}

void star::CommandBufferContainer::setToSubmitThisBuffer(const size_t& bufferIndex)
{
	assert(bufferIndex < this->allBuffers.size() && "Requested index does not exist");

	this->bufferSubmissionStatus[bufferIndex] = 1;
}

star::CommandBufferContainer::CompleteRequest& star::CommandBufferContainer::getBuffer(const star::Handle& bufferHandle)
{
	assert(bufferHandle.id < this->allBuffers.size() && "Requested index does not exist");

	return *this->allBuffers[bufferHandle.id];
}

void star::CommandBufferContainer::waitUntilOrderGroupReady(const int& frameIndex, const star::Command_Buffer_Order& order, const star::Command_Buffer_Type& type)
{
	auto waitResult = this->device.getDevice().waitForFences(this->bufferGroups[order]->fences[type].at(frameIndex), VK_TRUE, UINT64_MAX);
	if (waitResult != vk::Result::eSuccess) {
		throw std::runtime_error("Failed to wait for fence");
	}

	this->device.getDevice().resetFences(this->bufferGroups[order]->fences[type].at(frameIndex));
}

std::vector<std::reference_wrapper<star::CommandBufferContainer::CompleteRequest>> star::CommandBufferContainer::getAllBuffersOfTypeAndOrderReadyToSubmit(const star::Command_Buffer_Order& order, const star::Command_Buffer_Type& type, bool triggerReset)
{
	std::vector<std::reference_wrapper<CompleteRequest>> buffers;

	for (const auto& index : this->bufferGroups[order]->bufferOrderGroupsIndices[type]) {
		if (this->allBuffers[index]->type == type && (this->bufferSubmissionStatus[index] == 1 || this->bufferSubmissionStatus[index] == 2)) {
			if (triggerReset)
				this->resetThisBufferStatus(index);

			buffers.push_back(*this->allBuffers[index]);
		}
	}

	return buffers;
}

