#include "StarCommandBuffer.hpp"

star::StarCommandBuffer::StarCommandBuffer(vk::Device &vulkanDevice, int numBuffersToCreate,
                                           std::shared_ptr<StarCommandPool> parentPool, const star::Queue_Type type,
                                           bool initFences, bool initSemaphores)
    : vulkanDevice(vulkanDevice), parentPool(parentPool), type(type)
{
    this->waitSemaphores.resize(numBuffersToCreate);
    // allocate this from the pool
    vk::CommandBufferAllocateInfo allocateInfo = vk::CommandBufferAllocateInfo{};
    allocateInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
    allocateInfo.commandPool = this->parentPool->getVulkanCommandPool();
    allocateInfo.level = vk::CommandBufferLevel::ePrimary;
    allocateInfo.commandBufferCount = (uint32_t)numBuffersToCreate;

    this->commandBuffers = this->vulkanDevice.allocateCommandBuffers(allocateInfo);

    if (initFences)
        createFences();

    if (initSemaphores)
        createSemaphores();
}

star::StarCommandBuffer::~StarCommandBuffer()
{
    for (auto &fence : this->readyFence)
    {
        this->vulkanDevice.destroyFence(fence);
    }
    for (auto &semaphore : this->completeSemaphores)
    {
        this->vulkanDevice.destroySemaphore(semaphore);
    }
    this->vulkanDevice.freeCommandBuffers(this->parentPool->getVulkanCommandPool(), this->commandBuffers);
}

void star::StarCommandBuffer::begin(int buffIndex)
{
    assert(buffIndex < this->commandBuffers.size() && "Requested buffer does not exist");

    if (this->readyFence.size() > 0)
        wait(buffIndex);

    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;

    // flags parameter specifies command buffer use
    // VK_COMMAND_BUFFER_USEAGE_ONE_TIME_SUBMIT_BIT: command buffer recorded right after executing it once
    // VK_COMMAND_BUFFER_USEAGE_RENDER_PASS_CONTINUE_BIT: secondary command buffer that will be within a single render
    // pass VK_COMMAND_BUFFER_USEAGE_SIMULTANEOUS_USE_BIT: command buffer can be resubmitted while another instance has
    // already been submitted for execution
    beginInfo.flags = {};

    // only relevant for secondary command buffers -- which state to inherit from the calling primary command buffers
    beginInfo.pInheritanceInfo = nullptr;

    this->commandBuffers[buffIndex].begin(beginInfo);

    if (!this->commandBuffers[buffIndex])
    {
        throw std::runtime_error("Failed to begin recording command buffer");
    }
}

void star::StarCommandBuffer::begin(const int buffIndex, const vk::CommandBufferBeginInfo &beginInfo)
{
    assert(buffIndex < this->commandBuffers.size() && "Requested buffer does not exist");

    if (this->readyFence.size() > 0)
        wait(buffIndex);

    this->recorded = true;

    // create begin
    this->commandBuffers[buffIndex].begin(beginInfo);

    if (!this->commandBuffers[buffIndex])
    {
        throw std::runtime_error("Failed to begin recording command buffer");
    }
}

void star::StarCommandBuffer::waitFor(std::vector<vk::Semaphore> semaphores, vk::PipelineStageFlags whereWait)
{
    // check for double record
    for (auto &waits : this->waitSemaphores.at(0))
    {
        if (waits.first == semaphores.at(0))
        {
            return;
        }
    }

    for (int i = 0; i < semaphores.size(); i++)
    {
        this->waitSemaphores.at(i).push_back(
            std::pair<vk::Semaphore, vk::PipelineStageFlags>(semaphores.at(i), whereWait));
    }
}

void star::StarCommandBuffer::waitFor(StarCommandBuffer &otherBuffer, vk::PipelineStageFlags whereWait)
{
    this->waitFor(otherBuffer.getCompleteSemaphores(), whereWait);
}

void star::StarCommandBuffer::reset(int bufferIndex)
{
    assert(bufferIndex < this->commandBuffers.size() && "Requested buffer does not exist");

    // wait for fence before reset
    {
        auto result = this->vulkanDevice.waitForFences(this->readyFence[bufferIndex], VK_TRUE, UINT64_MAX);
        if (result != vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to wait for fence");
        }
    }

    // reset vulkan buffers
    this->commandBuffers.at(bufferIndex).reset();

    this->recorded = false;
}

void star::StarCommandBuffer::submit(int bufferIndex, vk::Queue &targetQueue, std::pair<vk::Semaphore, vk::PipelineStageFlags> *overrideWait, vk::Fence *overrideFence)
{
    std::vector<vk::Semaphore> waits = std::vector<vk::Semaphore>();
    std::vector<vk::PipelineStageFlags> waitPoints = std::vector<vk::PipelineStageFlags>();

    if (overrideWait != nullptr)
    {
        waits.push_back(overrideWait->first);
        waitPoints.push_back(overrideWait->second);
    }

    if (this->waitSemaphores.at(bufferIndex).size() > 0)
    {
        // there are some semaphores which must be waited on before execution
        for (auto &waitInfos : this->waitSemaphores.at(bufferIndex))
        {
            waits.push_back(waitInfos.first);
            waitPoints.push_back(waitInfos.second);
        }
    }

    std::vector<vk::Semaphore> signalSemaphores = std::vector<vk::Semaphore>();
    if (this->completeSemaphores.size() > 0)
    {
        signalSemaphores.push_back(this->completeSemaphores.at(bufferIndex));
    }

    vk::SubmitInfo submitInfo = vk::SubmitInfo()
        .setCommandBufferCount(1)
        .setPCommandBuffers(&this->commandBuffers.at(bufferIndex))
        .setWaitSemaphoreCount(waits.size() > 0 ? waits.size() : 0)
        .setPWaitSemaphores(waits.size() > 0 ? waits.data() : nullptr)
        .setPWaitDstStageMask(waitPoints.size() > 0 ? waitPoints.data() : nullptr)
        .setSignalSemaphoreCount(signalSemaphores.size() > 0 ? signalSemaphores.size() : 0)
        .setPSignalSemaphores(signalSemaphores.size() > 0 ? signalSemaphores.data() : 0);

    if (overrideFence != nullptr){
        targetQueue.submit(submitInfo, *overrideFence); 
    }else{
        targetQueue.submit(submitInfo, this->readyFence.at(bufferIndex)); 
    }

}

bool star::StarCommandBuffer::isFenceReady(const int &bufferIndex)
{
    assert(this->readyFence.size() > 0 && "No fences created");

    const auto result = this->vulkanDevice.getFenceStatus(this->readyFence.at(bufferIndex));
    return result == vk::Result::eSuccess;
}

std::vector<vk::Semaphore> &star::StarCommandBuffer::getCompleteSemaphores()
{
    if (this->completeSemaphores.size() == 0)
    {
        createSemaphores();
    }

    return this->completeSemaphores;
}

void star::StarCommandBuffer::wait(int bufferIndex)
{
    if (this->readyFence.size() > 0)
    {
        auto result = this->vulkanDevice.waitForFences(this->readyFence.at(bufferIndex), VK_TRUE, UINT64_MAX);

        if (result != vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to wait for fence");
        }

        this->vulkanDevice.resetFences(this->readyFence[bufferIndex]);
    }
}

void star::StarCommandBuffer::createSemaphores()
{
    this->completeSemaphores.resize(this->commandBuffers.size());

    vk::SemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = vk::StructureType::eSemaphoreCreateInfo;

    for (int i = 0; i < this->commandBuffers.size(); i++)
    {
        this->completeSemaphores.at(i) = this->vulkanDevice.createSemaphore(semaphoreInfo);
    }
}

void star::StarCommandBuffer::createTracking()
{
    createFences();
    createSemaphores();
}

void star::StarCommandBuffer::createFences()
{
    this->readyFence.resize(this->commandBuffers.size());

    vk::FenceCreateInfo fenceInfo{};
    fenceInfo.sType = vk::StructureType::eFenceCreateInfo;
    fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

    for (int i = 0; i < this->readyFence.size(); i++)
    {
        this->readyFence[i] = this->vulkanDevice.createFence(fenceInfo);
    }
}

std::vector<vk::CommandBuffer> star::StarCommandBuffer::CreateCommandBuffers(vk::Device &vulkanDevice,
                                                                             vk::CommandPool &commandPool,
                                                                             const uint32_t &numToCreate)
{
    // allocate this from the pool
    vk::CommandBufferAllocateInfo allocateInfo = vk::CommandBufferAllocateInfo()
                                                     .setCommandPool(commandPool)
                                                     .setLevel(vk::CommandBufferLevel::ePrimary)
                                                     .setCommandBufferCount(numToCreate);

    return vulkanDevice.allocateCommandBuffers(allocateInfo);
}