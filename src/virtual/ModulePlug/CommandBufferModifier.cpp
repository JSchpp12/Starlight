#include "CommandBufferModifier.hpp"

void star::CommandBufferModifier::setBufferHandle(Handle bufferHandle)
{
	this->bufferHandle = bufferHandle;
}

star::CommandBufferModifier::CommandBufferModifier()
{
	ManagerCommandBuffer::request(std::bind(&CommandBufferModifier::getCommandBufferRequest, this));
}

void star::CommandBufferModifier::submitMyBuffer()
{
	assert(this->bufferHandle.has_value() && "This buffer has not been assigned a handle");

	ManagerCommandBuffer::submitDynamicBuffer(this->bufferHandle.value());
}

std::optional<std::function<void(const int&)>> star::CommandBufferModifier::getAfterBufferSubmissionCallback()
{
	return std::optional<std::function<void(const int&)>>();
}

std::optional<std::function<void(const int&)>> star::CommandBufferModifier::getBeforeBufferSubmissionCallback()
{
	return std::optional<std::function<void(const int&)>>();
}

std::optional<std::function<void(star::StarCommandBuffer&, const int&)>> star::CommandBufferModifier::getOverrideBufferSubmissionCallback()
{
	return std::optional<std::function<void(StarCommandBuffer&, const int&)>>();
}

star::ManagerCommandBuffer::CommandBufferRequest star::CommandBufferModifier::getCommandBufferRequest()
{	
	return ManagerCommandBuffer::CommandBufferRequest(
		std::function<void(vk::CommandBuffer&, const int&)>(std::bind(&CommandBufferModifier::recordCommandBuffer, this, std::placeholders::_1, std::placeholders::_2)),
		std::function<void(Handle)>(std::bind(&CommandBufferModifier::setBufferHandle, this, std::placeholders::_1)),
		this->getCommandBufferOrder(), this->getCommandBufferType(), this->getWaitStages(),
		this->getWillBeSubmittedEachFrame(), this->getWillBeRecordedOnce(), 
		this->getBeforeBufferSubmissionCallback(), 
		this->getAfterBufferSubmissionCallback(), this->getOverrideBufferSubmissionCallback());
}
