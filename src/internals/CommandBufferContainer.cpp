#include "internals/CommandBufferContainer.hpp"

star::CommandBufferContainer::CommandBufferContainer(const int &numImagesInFlight, core::device::StarDevice &device)
    : bufferGroupsWithSubOrders(
          {std::make_pair(star::Command_Buffer_Order::before_render_pass, std::vector<CompleteRequest *>(5)),
           std::make_pair(star::Command_Buffer_Order::main_render_pass, std::vector<CompleteRequest *>(5)),
           std::make_pair(star::Command_Buffer_Order::after_render_pass, std::vector<CompleteRequest *>(5)),
           std::make_pair(star::Command_Buffer_Order::end_of_frame, std::vector<CompleteRequest *>(5))})
{

    for (int i = Command_Buffer_Order::before_render_pass; i != Command_Buffer_Order::presentation; i++)
    {
        this->bufferGroupsWithNoSubOrder[static_cast<star::Command_Buffer_Order>(i)] =
            std::make_unique<GenericBufferGroupInfo>(numImagesInFlight, device );
    }
}

void star::CommandBufferContainer::cleanup(core::device::StarDevice &device){
    for (auto &[order, group] : this->bufferGroupsWithNoSubOrder){
        group->cleanup(device);
    }
}

std::vector<vk::Semaphore> star::CommandBufferContainer::submitGroupWhenReady(core::device::StarDevice &device, 
    const star::Command_Buffer_Order &order, const uint8_t &frameInFlightIndex, const uint64_t &currentFrameIndex, std::vector<vk::Semaphore> *additionalWaitSemaphores)
{
    if (!this->subOrderSemaphoresUpToDate)
    {
        this->updateSemaphores();
        this->subOrderSemaphoresUpToDate = true;
    }

    std::vector<vk::Semaphore> semaphores = std::vector<vk::Semaphore>();

    // submit buffers which have a suborder first
    for (int i = star::Command_Buffer_Order_Index::first; i != star::Command_Buffer_Order_Index::fifth; i++)
    {
        if (this->bufferGroupsWithSubOrders[order][i - 1] == nullptr)
            break;

        CompleteRequest *buffer = this->bufferGroupsWithSubOrders[order][i - 1];

        if (buffer->beforeBufferSubmissionCallback.has_value())
            buffer->beforeBufferSubmissionCallback.value()(frameInFlightIndex);

        if (!buffer->recordOnce)
        {
            buffer->commandBuffer->begin(frameInFlightIndex);
            buffer->recordBufferCallback(buffer->commandBuffer->buffer(frameInFlightIndex), frameInFlightIndex, currentFrameIndex);
            buffer->commandBuffer->buffer(frameInFlightIndex).end();
        }

        buffer->submitCommandBuffer(device, frameInFlightIndex);

        if (i == star::Command_Buffer_Order_Index::fifth || this->bufferGroupsWithSubOrders[order][i] == nullptr)
        {
            semaphores.push_back(
                this->bufferGroupsWithSubOrders[order][i - 1]->commandBuffer->getCompleteSemaphores().at(frameInFlightIndex));
        }
    }

    // // submit all other buffers second
    // for (int type = star::Queue_Type::Tgraphics; type != star::Queue_Type::Tcompute; type++)
    // {
    //     std::vector<std::reference_wrapper<CompleteRequest>> buffersToSubmit =
    //         this->getAllBuffersOfTypeAndOrderReadyToSubmit(order, static_cast<star::Queue_Type>(type), true);

    //     if (!buffersToSubmit.empty())
    //     {
    //         waitUntilOrderGroupReady(device, frameInFlightIndex, order, static_cast<Queue_Type>(type));

    //         auto waitPoints = std::vector<vk::PipelineStageFlags>();
    //         for (auto &buffer : buffersToSubmit)
    //         {
    //             waitPoints.push_back(buffer.get().waitStage);
    //         }

    //         // before submission
    //         for (CompleteRequest &buffer : buffersToSubmit)
    //         {
    //             if (buffer.beforeBufferSubmissionCallback.has_value())
    //                 buffer.beforeBufferSubmissionCallback.value()(frameInFlightIndex);

    //             if (!buffer.recordOnce)
    //             {
    //                 buffer.commandBuffer->begin(frameInFlightIndex);
    //                 buffer.recordBufferCallback(buffer.commandBuffer->buffer(frameInFlightIndex), frameInFlightIndex);
    //                 buffer.commandBuffer->buffer(frameInFlightIndex).end();
    //             }
    //         }

    //         // submit
    //         {
    //             auto waitSemaphores = std::set<vk::Semaphore>(); 
    //             auto buffers = std::vector<vk::CommandBuffer>();
    //             if (additionalWaitSemaphores != nullptr){
    //                 for (auto& semaphore : *additionalWaitSemaphores){
    //                     waitSemaphores.insert(semaphore);
    //                 }
    //             }

    //             for (CompleteRequest &buffer : buffersToSubmit)
    //             {
    //                 buffers.push_back(buffer.commandBuffer->buffer(frameInFlightIndex));

    //                 for (auto& semaphore : buffer.getDependentHighPrioritySemaphores(frameInFlightIndex)){
    //                     waitSemaphores.insert(semaphore); 
    //                     waitPoints.push_back(buffer.waitStage); 
    //                 }
    //             }

    //             auto submitInfo = vk::SubmitInfo{};

    //             auto semaphoreData = std::vector<vk::Semaphore>(); 
    //             for (auto &info : waitSemaphores){
    //                 semaphoreData.push_back(info); 
    //             }

    //             assert(waitPoints.size() == semaphoreData.size() && "Each semaphore needs a wait stage"); 
                
    //                 // waitSemaphores = std::vector<vk::Semaphore>(*additionalWaitSemaphores);
    //                 // submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores->size());
    //                 // submitInfo.pWaitSemaphores = waitSemaphores->data();
    //                 // submitInfo.pWaitDstStageMask = waitPoints.data();

    //             uint32_t waitCount = 0; 
    //             CastHelpers::SafeCast<size_t, uint32_t>(semaphoreData.size(), waitCount); 

    //             submitInfo.pWaitSemaphores = semaphoreData.data(); 
    //             submitInfo.waitSemaphoreCount = waitCount;
    //             submitInfo.pWaitDstStageMask = waitPoints.data(); 
    //             submitInfo.signalSemaphoreCount = 1;
    //             submitInfo.pSignalSemaphores =
    //                 &this->bufferGroupsWithNoSubOrder[order]->semaphores[static_cast<Queue_Type>(type)].at(
    //                     frameInFlightIndex);

    //             // record for later
    //             semaphores.push_back(
    //                 this->bufferGroupsWithNoSubOrder[order]->semaphores[static_cast<Queue_Type>(type)].at(
    //                     frameInFlightIndex));

    //             submitInfo.pCommandBuffers = buffers.data();
    //             submitInfo.commandBufferCount = buffers.size();

    //             std::unique_ptr<vk::Result> commandResult = std::unique_ptr<vk::Result>();
    //             vk::Fence workingFence =
    //                 this->bufferGroupsWithNoSubOrder[order]->fences[static_cast<Queue_Type>(type)].at(
    //                     frameInFlightIndex);
    //             commandResult = std::make_unique<vk::Result>(device.getDefaultQueue(static_cast<Queue_Type>(type))
    //                                                              .getVulkanQueue()
    //                                                              .submit(1, &submitInfo, workingFence));

    //             assert(commandResult != nullptr && "Invalid command buffer type");

    //             if (*commandResult.get() != vk::Result::eSuccess)
    //             {
    //                 throw std::runtime_error("Failed to submit command buffer");
    //             }
    //         }
    //     }
    // }

    return semaphores;
}

star::Handle star::CommandBufferContainer::add(
    std::unique_ptr<star::CommandBufferContainer::CompleteRequest> newRequest, const bool &willBeSubmittedEachFrame,
    const star::Queue_Type &type, const star::Command_Buffer_Order &order,
    const star::Command_Buffer_Order_Index &subOrder)
{
    uint32_t count = 0;
    CastHelpers::SafeCast<size_t, uint32_t>(this->allBuffers.size(), count);

    star::Handle newHandle = star::Handle(star::Handle_Type::buffer, count);
    const int bufferIndex = this->allBuffers.size();

    this->allBuffers.push_back(std::move(newRequest));
    this->bufferSubmissionStatus.push_back(willBeSubmittedEachFrame ? 2 : 0);

    if (subOrder != Command_Buffer_Order_Index::dont_care)
    {
        assert(subOrder != 0 && "This should never happen");
        this->subOrderSemaphoresUpToDate = false;
        this->bufferGroupsWithSubOrders[order][static_cast<int>(subOrder) - 1] = this->allBuffers[bufferIndex].get();
    }
    else
    {
        this->bufferGroupsWithNoSubOrder[order]->bufferOrderGroupsIndices[type].push_back(bufferIndex);
    }

    return newHandle;
}

bool star::CommandBufferContainer::shouldSubmitThisBuffer(const size_t &bufferIndex)
{
    assert(bufferIndex < this->allBuffers.size() && "Requested index does not exist");

    return this->bufferSubmissionStatus[bufferIndex] & 1 || this->bufferSubmissionStatus[bufferIndex] & 2;
}

void star::CommandBufferContainer::resetThisBufferStatus(const size_t &bufferIndex)
{
    assert(bufferIndex < this->allBuffers.size() && "Requested index does not exist");

    if (this->bufferSubmissionStatus[bufferIndex] & 1)
        this->bufferSubmissionStatus[bufferIndex] = 0;
}

void star::CommandBufferContainer::setToSubmitThisBuffer(const size_t &bufferIndex)
{
    assert(bufferIndex < this->allBuffers.size() && "Requested index does not exist");

    this->bufferSubmissionStatus[bufferIndex] = 1;
}

star::CommandBufferContainer::CompleteRequest &star::CommandBufferContainer::get(const star::Handle &bufferHandle)
{
    assert(bufferHandle.getID() < this->allBuffers.size() && "Requested index does not exist");

    return *this->allBuffers[bufferHandle.getID()];
}

void star::CommandBufferContainer::waitUntilOrderGroupReady(core::device::StarDevice &device, const int &frameIndex,
                                                            const star::Command_Buffer_Order &order,
                                                            const star::Queue_Type &type)
{
    auto waitResult = device.getVulkanDevice().waitForFences(
        this->bufferGroupsWithNoSubOrder[order]->fences[type].at(frameIndex), VK_TRUE, UINT64_MAX);
    if (waitResult != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to wait for fence");
    }

    device.getVulkanDevice().resetFences(this->bufferGroupsWithNoSubOrder[order]->fences[type].at(frameIndex));
}

std::vector<std::reference_wrapper<star::CommandBufferContainer::CompleteRequest>> star::CommandBufferContainer::
    getAllBuffersOfTypeAndOrderReadyToSubmit(const star::Command_Buffer_Order &order, const star::Queue_Type &type,
                                             bool triggerReset)
{
    std::vector<std::reference_wrapper<CompleteRequest>> buffers;

    for (const auto &index : this->bufferGroupsWithNoSubOrder[order]->bufferOrderGroupsIndices[type])
    {
        if (this->allBuffers[index]->type == type &&
            (this->bufferSubmissionStatus[index] == 1 || this->bufferSubmissionStatus[index] == 2))
        {
            if (triggerReset)
                this->resetThisBufferStatus(index);

            buffers.push_back(*this->allBuffers[index]);
        }
    }

    return buffers;
}

void star::CommandBufferContainer::updateSemaphores()
{
    for (int i = star::Command_Buffer_Order::before_render_pass; i != Command_Buffer_Order::presentation; i++)
    {
        if (this->bufferGroupsWithSubOrders[static_cast<Command_Buffer_Order>(i)][0] != nullptr)
        {
            for (int j = Command_Buffer_Order_Index::first; j < Command_Buffer_Order_Index::fifth; j++)
            {
                auto *currentBuffer = this->bufferGroupsWithSubOrders[static_cast<Command_Buffer_Order>(i)][j - 1];
                auto *nextBuffer = this->bufferGroupsWithSubOrders[static_cast<Command_Buffer_Order>(i)][j];

                if (nextBuffer != nullptr)
                {
                    nextBuffer->commandBuffer->waitFor(currentBuffer->commandBuffer->getCompleteSemaphores(),
                                                       nextBuffer->waitStage);
                }
            }
        }
    }
}