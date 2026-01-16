#include "StarCommandBuffer.hpp"

#include <star_common/helper/CastHelpers.hpp>

star::StarCommandBuffer::StarCommandBuffer(vk::Device &vulkanDevice, int numBuffersToCreate,
                                           const StarCommandPool *parentPool, const star::Queue_Type type,
                                           bool initFences, bool initSemaphores)
    : vulkanDevice(vulkanDevice), parentPool(parentPool), type(type)
{
    this->waitSemaphores.resize(numBuffersToCreate);
    vk::CommandBufferAllocateInfo allocateInfo = vk::CommandBufferAllocateInfo()
                                                     .setCommandPool(parentPool->getVulkanCommandPool())
                                                     .setLevel(vk::CommandBufferLevel::ePrimary)
                                                     .setCommandBufferCount(static_cast<uint32_t>(numBuffersToCreate));

    this->commandBuffers = this->vulkanDevice.allocateCommandBuffers(allocateInfo);

    if (initFences)
        createFences();

    if (initSemaphores)
        createSemaphores();
}

star::StarCommandBuffer::~StarCommandBuffer()
{
    if (commandBuffers.size() > 0)
    {
        cleanupRender(vulkanDevice);
    }
}

void star::StarCommandBuffer::cleanupRender(vk::Device &device)
{
    for (auto &fence : this->readyFence)
    {
        device.destroyFence(fence);
    }
    readyFence.clear();

    for (auto &semaphore : this->completeSemaphores)
    {
        device.destroySemaphore(semaphore);
    }
    completeSemaphores.clear();

    if (commandBuffers.size() > 0)
    {
        device.freeCommandBuffers(this->parentPool->getVulkanCommandPool(), this->commandBuffers);
    }
    commandBuffers.clear();
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

void star::StarCommandBuffer::submit(int bufferIndex, vk::Queue &targetQueue,
                                     std::vector<std::pair<vk::Semaphore, vk::PipelineStageFlags>> *overrideWait,
                                     std::vector<std::optional<uint64_t>> *additionalWaitsSignaledValues,
                                     vk::Fence *overrideFence, std::vector<vk::Semaphore> *additionalSignalSemaphores)
{
    std::vector<vk::Semaphore> waits = std::vector<vk::Semaphore>();
    std::vector<vk::PipelineStageFlags> waitPoints = std::vector<vk::PipelineStageFlags>();
    std::vector<uint64_t> waitSignalValues;

    if (overrideWait != nullptr)
    {
        for (size_t i{0}; i < overrideWait->size(); i++)
        {
            waits.push_back(overrideWait->at(i).first);
            waitPoints.push_back(overrideWait->at(i).second);
            waitSignalValues.push_back(
                additionalWaitsSignaledValues->at(i).has_value() ? additionalWaitsSignaledValues->at(i).value() : 0);
        }
    }

    if (this->waitSemaphores.at(bufferIndex).size() > 0)
    {
        // there are some semaphores which must be waited on before execution
        for (auto &waitInfos : this->waitSemaphores.at(bufferIndex))
        {
            waits.push_back(waitInfos.first);
            waitPoints.push_back(waitInfos.second);
            waitSignalValues.push_back(0); // star command buffers themselves do not use timeline semaphores
        }
    }

    std::vector<vk::Semaphore> signalSemaphores = std::vector<vk::Semaphore>();
    if (this->completeSemaphores.size() > 0)
    {
        signalSemaphores.push_back(this->completeSemaphores.at(bufferIndex));
    }

    if (additionalSignalSemaphores != nullptr)
    {
        for (const auto &semaphore : *additionalSignalSemaphores)
        {
            signalSemaphores.push_back(semaphore);
        }
    }

    uint32_t signalSemaphoreCount = 0;
    if (!common::helper::SafeCast<size_t, uint32_t>(signalSemaphores.size(), signalSemaphoreCount))
    {
        throw std::runtime_error("Failed to cast signal semaphore counts");
    }

    uint32_t waitValueCount = 0;
    common::helper::SafeCast(waitSignalValues.size(), waitValueCount);

    const vk::TimelineSemaphoreSubmitInfo time = vk::TimelineSemaphoreSubmitInfo()
                                                     .setWaitSemaphoreValueCount(waitValueCount)
                                                     .setWaitSemaphoreValues(waitSignalValues);

    vk::SubmitInfo submitInfo = vk::SubmitInfo()
                                    .setPNext(&time)
                                    .setCommandBufferCount(1)
                                    .setPCommandBuffers(&this->commandBuffers.at(bufferIndex))
                                    .setWaitSemaphoreCount(waits.size() > 0 ? waits.size() : 0)
                                    .setPWaitSemaphores(waits.size() > 0 ? waits.data() : nullptr)
                                    .setPWaitDstStageMask(waitPoints.size() > 0 ? waitPoints.data() : nullptr)
                                    .setSignalSemaphoreCount(signalSemaphoreCount)
                                    .setPSignalSemaphores(signalSemaphores.size() > 0 ? signalSemaphores.data() : 0);

    if (overrideFence != nullptr)
    {
        targetQueue.submit(submitInfo, *overrideFence);
    }
    else if (this->readyFence.size() > 0)
    {
        targetQueue.submit(submitInfo, this->readyFence.at(bufferIndex));
    }
    else
    {
        targetQueue.submit(submitInfo);
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

    for (size_t i = 0; i < this->commandBuffers.size(); i++)
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

    for (size_t i = 0; i < this->readyFence.size(); i++)
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