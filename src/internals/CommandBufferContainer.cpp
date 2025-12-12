#include "internals/CommandBufferContainer.hpp"

#include <syncstream>

star::CommandBufferContainer::CommandBufferContainer(const int &numImagesInFlight, core::device::StarDevice &device)
    : bufferGroupsWithSubOrders({std::make_pair(star::Command_Buffer_Order::before_render_pass, std::vector<Handle>(5)),
                                 std::make_pair(star::Command_Buffer_Order::main_render_pass, std::vector<Handle>(5)),
                                 std::make_pair(star::Command_Buffer_Order::after_render_pass, std::vector<Handle>(5)),
                                 std::make_pair(star::Command_Buffer_Order::end_of_frame, std::vector<Handle>(5))})
{
}

std::vector<vk::Semaphore> star::CommandBufferContainer::submitGroupWhenReady(
    core::device::StarDevice &device, const star::Command_Buffer_Order &order, const uint8_t &frameInFlightIndex,
    const uint64_t &currentFrameIndex, std::vector<vk::Semaphore> *additionalWaitSemaphores)
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
        if (!bufferGroupsWithSubOrders[order][i - 1].isInitialized())
        {
            break;
        }

        if (shouldSubmitThisBuffer(bufferGroupsWithSubOrders[order][i - 1]))
        {
            CompleteRequest *buffer = this->allBuffers[bufferGroupsWithSubOrders[order][i - 1].getID()].get();

            if (buffer->beforeBufferSubmissionCallback.has_value())
                buffer->beforeBufferSubmissionCallback.value()(frameInFlightIndex);

            if (!buffer->recordOnce)
            {
                buffer->commandBuffer->begin(frameInFlightIndex);
                buffer->recordBufferCallback(buffer->commandBuffer->buffer(frameInFlightIndex), frameInFlightIndex,
                                             currentFrameIndex);
                buffer->commandBuffer->buffer(frameInFlightIndex).end();
            }

            vk::Semaphore doneSemaphore;
            if (i == star::Command_Buffer_Order_Index::first)
            {
                doneSemaphore = buffer->submitCommandBuffer(device, frameInFlightIndex, additionalWaitSemaphores);
            }
            else
            {
                doneSemaphore = buffer->submitCommandBuffer(device, frameInFlightIndex);
            }

            if (i == star::Command_Buffer_Order_Index::fifth ||
                !this->bufferGroupsWithSubOrders[order][i].isInitialized())
            {
                semaphores.push_back(doneSemaphore);
            }

            resetThisBufferStatus(bufferGroupsWithSubOrders[order][i - 1]);
        }
    }

    return semaphores;
}

star::Handle star::CommandBufferContainer::add(
    std::shared_ptr<star::CommandBufferContainer::CompleteRequest> newRequest, const bool &willBeSubmittedEachFrame,
    const star::Queue_Type &type, const star::Command_Buffer_Order &order,
    const star::Command_Buffer_Order_Index &subOrder)
{
    uint32_t count = 0;
    common::helper::SafeCast<size_t, uint32_t>(this->allBuffers.size(), count);

    star::Handle newHandle{.type = common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
                               common::special_types::CommandBufferTypeName),
                           .id = count};
    const int bufferIndex = this->allBuffers.size();

    this->allBuffers.push_back(std::move(newRequest));
    this->bufferSubmissionStatus.push_back(willBeSubmittedEachFrame ? 2 : 0);

    assert(subOrder != 0 && "This should never happen");
    this->subOrderSemaphoresUpToDate = false;
    this->bufferGroupsWithSubOrders[order][static_cast<int>(subOrder) - 1] = newHandle;

    return newHandle;
}

bool star::CommandBufferContainer::shouldSubmitThisBuffer(const Handle &buffer)
{
    assert(buffer.getID() < this->allBuffers.size() && "Requested index does not exist");

    return this->bufferSubmissionStatus[buffer.getID()] & 1 || this->bufferSubmissionStatus[buffer.getID()] & 2;
}

void star::CommandBufferContainer::resetThisBufferStatus(const Handle &buffer)
{
    assert(buffer.getID() < this->allBuffers.size() && "Requested index does not exist");

    if (this->bufferSubmissionStatus[buffer.getID()] & 1)
        this->bufferSubmissionStatus[buffer.getID()] = 0;
}

void star::CommandBufferContainer::setToSubmitThisBuffer(const Handle &buffer)
{
    assert(buffer.getID() < this->allBuffers.size() && "Requested index does not exist");

    this->bufferSubmissionStatus[buffer.getID()] = 1;
}

star::CommandBufferContainer::CompleteRequest &star::CommandBufferContainer::get(const star::Handle &bufferHandle)
{
    assert(bufferHandle.getID() < this->allBuffers.size() && "Requested index does not exist");

    return *this->allBuffers[bufferHandle.getID()];
}

void star::CommandBufferContainer::updateSemaphores()
{
    for (int i = star::Command_Buffer_Order::before_render_pass; i != Command_Buffer_Order::presentation; i++)
    {
        if (this->bufferGroupsWithSubOrders[static_cast<Command_Buffer_Order>(i)][0].isInitialized())
        {
            for (int j = 0; j < bufferGroupsWithSubOrders[static_cast<Command_Buffer_Order>(i)].size(); j++)
            {
                auto currentBuffer =
                    allBuffers[bufferGroupsWithSubOrders[static_cast<Command_Buffer_Order>(i)][j].getID()];
                const int nextIndex = j + 1;
                const auto &nextBufferHandle = bufferGroupsWithSubOrders[static_cast<Command_Buffer_Order>(i)][nextIndex];

                if (nextBufferHandle.isInitialized())
                {
                    auto *nextBuffer = allBuffers[nextBufferHandle.getID()].get();
                    nextBuffer->commandBuffer->waitFor(currentBuffer->commandBuffer->getCompleteSemaphores(),
                                                       nextBuffer->waitStage);
                }else{
                    break;
                }
            }
        }
    }
}