#include "StarCommandBuffer.hpp"

star::StarCommandBuffer::StarCommandBuffer(StarDevice& device, int numBuffersToCreate, 
	star::Command_Buffer_Type type) : device(device),
	targetQueue(device.getQueue(type))
{
	this->waitSemaphores.resize(numBuffersToCreate); 

	vk::CommandPool& pool = device.getCommandPool(type); 

	//allocate this from the pool
	vk::CommandBufferAllocateInfo allocateInfo = vk::CommandBufferAllocateInfo{}; 
	allocateInfo.sType = vk::StructureType::eCommandBufferAllocateInfo; 
	allocateInfo.commandPool = pool; 
	allocateInfo.level = vk::CommandBufferLevel::ePrimary; 
	allocateInfo.commandBufferCount =(uint32_t)numBuffersToCreate; 
	
	this->commandBuffers = this->device.getDevice().allocateCommandBuffers(allocateInfo); 

}

star::StarCommandBuffer::~StarCommandBuffer()
{
	for (auto& semaphore : this->completeSemaphores) {
		this->device.getDevice().destroySemaphore(semaphore); 
	}
}

vk::CommandBuffer& star::StarCommandBuffer::begin(int buffIndex)
{
	vk::CommandBufferBeginInfo beginInfo{};
	//beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;

	//flags parameter specifies command buffer use 
		//VK_COMMAND_BUFFER_USEAGE_ONE_TIME_SUBMIT_BIT: command buffer recorded right after executing it once
		//VK_COMMAND_BUFFER_USEAGE_RENDER_PASS_CONTINUE_BIT: secondary command buffer that will be within a single render pass 
		//VK_COMMAND_BUFFER_USEAGE_SIMULTANEOUS_USE_BIT: command buffer can be resubmitted while another instance has already been submitted for execution
	beginInfo.flags = {};

	//only relevant for secondary command buffers -- which state to inherit from the calling primary command buffers 
	beginInfo.pInheritanceInfo = nullptr;

	/* NOTE:
		if the command buffer has already been recorded once, simply call vkBeginCommandBuffer->implicitly reset.
		commands cannot be added after creation
	*/

	return this->begin(buffIndex, beginInfo); 
}

void star::StarCommandBuffer::submit(int targetIndex, vk::Fence& fence)
{
	assert(this->recorded && "Buffer should be recorded before submission"); 

	vk::SubmitInfo submitInfo{}; 

	std::vector<vk::Semaphore> waits;
	std::vector<vk::PipelineStageFlags> whereWaits;
	if (this->waitSemaphores.size() > 0) {
		//there are some semaphores which must be waited on before execution
		for (auto& waitInfos : this->waitSemaphores.at(targetIndex)) {
			waits.push_back(waitInfos.first); 
			whereWaits.push_back(waitInfos.second); 
		}
	}
	submitInfo.waitSemaphoreCount = (uint32_t)waits.size();
	submitInfo.pWaitSemaphores = waits.data();
	submitInfo.pWaitDstStageMask = whereWaits.data(); 

	//check if need to signal complete semaphore
	if (this->completeSemaphores.size() > 0) {
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &this->completeSemaphores.at(targetIndex); 
	}

	submitInfo.pCommandBuffers = &this->commandBuffers.at(targetIndex); 
	submitInfo.commandBufferCount = 1; 
	auto commandResult = this->targetQueue.submit(1, &submitInfo, fence); 
	if (commandResult != vk::Result::eSuccess) {
		throw std::runtime_error("failed to submit draw command buffer");
	}
}

void star::StarCommandBuffer::submit(int targetIndex, vk::Fence& fence, std::pair<vk::Semaphore, vk::PipelineStageFlags> overrideWait)
{
	assert(this->recorded && "Buffer should be recorded before submission");

	vk::SubmitInfo submitInfo{};

	std::vector<vk::Semaphore> waits{
		overrideWait.first
	};
	std::vector<vk::PipelineStageFlags> whereWaits{
		overrideWait.second
	};

	if (this->waitSemaphores.size() > 0) {
		//there are some semaphores which must be waited on before execution
		for (auto& waitInfos : this->waitSemaphores.at(targetIndex)) {
			waits.push_back(waitInfos.first);
			whereWaits.push_back(waitInfos.second);
		}
	}
	submitInfo.waitSemaphoreCount = (uint32_t)waits.size();
	submitInfo.pWaitSemaphores = waits.data(); 
	submitInfo.pWaitDstStageMask = whereWaits.data(); 
	//check if need to signal complete semaphore
	if (this->completeSemaphores.size() > 0) {
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &this->completeSemaphores.at(targetIndex);
	}

	submitInfo.pCommandBuffers = &this->commandBuffers.at(targetIndex);
	submitInfo.commandBufferCount = 1;
	auto commandResult = this->targetQueue.submit(1, &submitInfo, fence);
	if (commandResult != vk::Result::eSuccess) {
		throw std::runtime_error("failed to submit draw command buffer");
	}
}

void star::StarCommandBuffer::waitFor(std::vector<vk::Semaphore> semaphores, vk::PipelineStageFlags whereWait)
{
	//check for double record 
	for (auto& waits : this->waitSemaphores.at(0)) {
		if (waits.first == semaphores.at(0)) {
			std::cout << "Duplicate request for wait semaphore detected...skipping" << std::endl;
			return; 
		}
	}

	for (int i = 0; i < semaphores.size(); i++) {
		this->waitSemaphores.at(i).push_back(std::pair<vk::Semaphore, vk::PipelineStageFlags>(semaphores.at(i), whereWait)); 
	}
}

const std::vector<vk::Semaphore>& star::StarCommandBuffer::getCompleteSemaphores()
{
	if (this->completeSemaphores.size() == 0) {
		//need to create semaphores
		this->completeSemaphores.resize(this->commandBuffers.size());

		vk::SemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = vk::StructureType::eSemaphoreCreateInfo;

		for (int i = 0; i < this->commandBuffers.size(); i++) {
			this->completeSemaphores.at(i) = this->device.getDevice().createSemaphore(semaphoreInfo);
		}
	}

	return this->completeSemaphores;
}

vk::CommandBuffer& star::StarCommandBuffer::begin(int buffIndex, vk::CommandBufferBeginInfo beginInfo)
{
	assert(buffIndex < this->commandBuffers.size() && "Requested swap chain index is too high");
	this->recorded = true; 

	//create begin 
	this->commandBuffers[buffIndex].begin(beginInfo); 

	if (!this->commandBuffers[buffIndex]) {
		throw std::runtime_error("failed to begin recording command buffer"); 
	}
	return this->commandBuffers[buffIndex]; 
}
