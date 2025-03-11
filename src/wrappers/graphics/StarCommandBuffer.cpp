#include "StarCommandBuffer.hpp"

star::StarCommandBuffer::StarCommandBuffer(StarDevice& device, int numBuffersToCreate, 
	star::Queue_Type type, bool initFences, bool initSemaphores)
	: device(device), targetQueue(device.getQueueFamily(type).getQueue())
{
	this->recordedImageTransitions.resize(numBuffersToCreate); 
	for (auto& empty : this->recordedImageTransitions) {
		empty = std::make_unique<std::unordered_map<StarImage*, std::pair<vk::ImageLayout, vk::ImageLayout>>>();
	}
	this->waitSemaphores.resize(numBuffersToCreate); 

	vk::CommandPool& pool = device.getQueueFamily(type).getCommandPool(); 

	//allocate this from the pool
	vk::CommandBufferAllocateInfo allocateInfo = vk::CommandBufferAllocateInfo{}; 
	allocateInfo.sType = vk::StructureType::eCommandBufferAllocateInfo; 
	allocateInfo.commandPool = pool; 
	allocateInfo.level = vk::CommandBufferLevel::ePrimary; 
	allocateInfo.commandBufferCount = (uint32_t)numBuffersToCreate; 
	
	this->commandBuffers = this->device.getDevice().allocateCommandBuffers(allocateInfo);  

	if (initFences) 
		createFences(); 
	
	if (initSemaphores)
		createSemaphores(); 
}

star::StarCommandBuffer::~StarCommandBuffer()
{
	for (auto& fence : this->readyFence) {
		if (fence)
			this->device.getDevice().destroyFence(fence);
	}
	for (auto& semaphore : this->completeSemaphores) {
		if (semaphore)
			this->device.getDevice().destroySemaphore(semaphore); 
	}
}

void star::StarCommandBuffer::begin(int buffIndex)
{
	if (this->readyFence.size() > 0)
		wait(buffIndex);

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

	this->begin(buffIndex, beginInfo); 
}

void star::StarCommandBuffer::begin(int buffIndex, vk::CommandBufferBeginInfo beginInfo)
{
	assert(buffIndex < this->commandBuffers.size() && "Requested swap chain index is too high");
	this->recorded = true;

	//create begin 
	this->commandBuffers[buffIndex].begin(beginInfo);

	if (!this->commandBuffers[buffIndex]) {
		throw std::runtime_error("failed to begin recording command buffer");
	}
}

void star::StarCommandBuffer::submit(int bufferIndex){
	assert(this->recorded && "Buffer should be recorded before submission");

	vk::SubmitInfo submitInfo{}; 

	std::vector<vk::Semaphore> waits; 
	std::vector<vk::PipelineStageFlags> waitPoints; 

	if (this->waitSemaphores.at(bufferIndex).size() > 0) {
		//there are some semaphores which must be waited on before execution
		for (auto& waitInfos : this->waitSemaphores.at(bufferIndex)) {
			waits.push_back(waitInfos.first);
			waitPoints.push_back(waitInfos.second);
		}
	}

	if (waits.size() > 0){
		submitInfo.waitSemaphoreCount = (uint32_t)waits.size();
		submitInfo.pWaitSemaphores = waits.data(); 
		submitInfo.pWaitDstStageMask = waitPoints.data(); 
	}

	//check if need to signal complete semaphore
	if (this->completeSemaphores.size() > 0) {
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &this->completeSemaphores.at(bufferIndex);
	}

	submitInfo.pCommandBuffers = &this->commandBuffers.at(bufferIndex);
	submitInfo.commandBufferCount = 1;

	auto commandResult = this->targetQueue.submit(1, &submitInfo, this->readyFence[bufferIndex]);
	if (commandResult != vk::Result::eSuccess) {
		throw std::runtime_error("failed to submit draw command buffer");
	}
}

void star::StarCommandBuffer::submit(int bufferIndex, vk::Fence& fence)
{
	assert(this->recorded && "Buffer should be recorded before submission"); 

	vk::SubmitInfo submitInfo{}; 

	std::vector<vk::Semaphore> waits; 
	std::vector<vk::PipelineStageFlags> waitPoints; 

	if (this->waitSemaphores.at(bufferIndex).size() > 0) {
		//there are some semaphores which must be waited on before execution
		for (auto& waitInfos : this->waitSemaphores.at(bufferIndex)) {
			waits.push_back(waitInfos.first);
			waitPoints.push_back(waitInfos.second);
		}
	}

	if (waits.size() > 0){
		submitInfo.waitSemaphoreCount = (uint32_t)waits.size();
		submitInfo.pWaitSemaphores = waits.data(); 
		submitInfo.pWaitDstStageMask = waitPoints.data(); 
	}

	//check if need to signal complete semaphore
	if (this->completeSemaphores.size() > 0) {
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &this->completeSemaphores.at(bufferIndex);
	}

	submitInfo.pCommandBuffers = &this->commandBuffers.at(bufferIndex);
	submitInfo.commandBufferCount = 1;
	auto commandResult = this->targetQueue.submit(1, &submitInfo, fence); 
	if (commandResult != vk::Result::eSuccess) {
		throw std::runtime_error("failed to submit draw command buffer");
	}
}

void star::StarCommandBuffer::submit(int bufferIndex, vk::Fence& fence, std::pair<vk::Semaphore, vk::PipelineStageFlags> overrideWait)
{
	assert(this->recorded && "Buffer should be recorded before submission");

	std::vector<vk::Semaphore> waits{overrideWait.first}; 
	std::vector<vk::PipelineStageFlags> waitPoints{overrideWait.second}; 

	vk::SubmitInfo submitInfo{};

	if (this->waitSemaphores.at(bufferIndex).size() > 0) {
		//there are some semaphores which must be waited on before execution
		for (auto& waitInfos : this->waitSemaphores.at(bufferIndex)) {
			waits.push_back(waitInfos.first);
			waitPoints.push_back(waitInfos.second);
		}
	}

	if (waits.size() > 0){
		submitInfo.waitSemaphoreCount = (uint32_t)waits.size();
		submitInfo.pWaitSemaphores = waits.data(); 
		submitInfo.pWaitDstStageMask = waitPoints.data(); 
	}

	//check if need to signal complete semaphore
	if (this->completeSemaphores.size() > 0) {
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &this->completeSemaphores.at(bufferIndex);
	}

	submitInfo.pCommandBuffers = &this->commandBuffers.at(bufferIndex);
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

void star::StarCommandBuffer::waitFor(StarCommandBuffer& otherBuffer, vk::PipelineStageFlags whereWait)
{
	this->waitFor(otherBuffer.getCompleteSemaphores(), whereWait);
}

void star::StarCommandBuffer::reset(int bufferIndex)
{
	//wait for fence before reset
	this->device.getDevice().waitForFences(this->readyFence[bufferIndex], VK_TRUE, UINT64_MAX);

	//reset image transitions
	this->recordedImageTransitions.at(bufferIndex).reset();
	this->recordedImageTransitions.at(bufferIndex) = std::make_unique<std::unordered_map<StarImage*, std::pair<vk::ImageLayout, vk::ImageLayout>>>();

	//reset vulkan buffers
	this->commandBuffers.at(bufferIndex).reset();

	this->recorded = false; 
}

std::vector<vk::Semaphore>& star::StarCommandBuffer::getCompleteSemaphores()
{
	if (this->completeSemaphores.size() == 0) {
		createSemaphores();
	}

	return this->completeSemaphores;
}

void star::StarCommandBuffer::wait(int bufferIndex)
{
	this->device.getDevice().waitForFences(this->readyFence.at(bufferIndex), VK_TRUE, UINT64_MAX); 

	this->device.getDevice().resetFences(this->readyFence[bufferIndex]); 
}

void star::StarCommandBuffer::checkForImageTransitions(int bufferIndex)
{
	//crude - update images with the transforms that this queue will enforce -- just in case the buffer is not re-recorded each frame 
	//WARNING: if multithreading is used, this tracking system will NOT work

	//for the images that have transitions, manually update their layout in the event this buffer is not recorded each frame
	for (auto& transitions : *this->recordedImageTransitions.at(bufferIndex).get()) {
		if (transitions.first->getCurrentLayout() != transitions.second.first)
			std::cout << "Warning: iamge not in expected format. This might cause undefined behavior from renderer" << std::endl;

		transitions.first->overrideImageLayout(transitions.second.second); 
	}
}

void star::StarCommandBuffer::createSemaphores()
{
	this->completeSemaphores.resize(this->commandBuffers.size());

	vk::SemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = vk::StructureType::eSemaphoreCreateInfo;

	for (int i = 0; i < this->commandBuffers.size(); i++) {
		this->completeSemaphores.at(i) = this->device.getDevice().createSemaphore(semaphoreInfo);
	}
}

void star::StarCommandBuffer::createTracking()
{
	createFences(); 
	createSemaphores();
}

void star::StarCommandBuffer::createFences() {
	this->readyFence.resize(this->commandBuffers.size());

	vk::FenceCreateInfo fenceInfo{};
	fenceInfo.sType = vk::StructureType::eFenceCreateInfo;
	fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

	for (int i = 0; i < this->readyFence.size(); i++) {
		this->readyFence[i] = this->device.getDevice().createFence(fenceInfo);
	}
}