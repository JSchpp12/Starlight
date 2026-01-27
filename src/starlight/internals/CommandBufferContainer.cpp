#include "internals/CommandBufferContainer.hpp"

star::CommandBufferContainer::CommandBufferContainer(core::device::StarDevice &device, const uint8_t &numImagesInFlight)
    : bufferGroupsWithSubOrders({std::make_pair(star::Command_Buffer_Order::before_render_pass, std::vector<Handle>(5)),
                                 std::make_pair(star::Command_Buffer_Order::main_render_pass, std::vector<Handle>(5)),
                                 std::make_pair(star::Command_Buffer_Order::after_render_pass, std::vector<Handle>(5)),
                                 std::make_pair(star::Command_Buffer_Order::end_of_frame, std::vector<Handle>(5)),
                                 std::make_pair(star::Command_Buffer_Order::presentation, std::vector<Handle>(1))})
{
}

std::vector<vk::Semaphore> star::CommandBufferContainer::submitGroupWhenReady(
    core::device::StarDevice &device, const star::Command_Buffer_Order &order, const common::FrameTracker &frameTracker,
    const uint64_t &currentFrameIndex, absl::flat_hash_map<star::Queue_Type, StarQueue *> &queues,
    std::vector<vk::Semaphore> *additionalWaitSemaphores)
{
    if (!this->subOrderSemaphoresUpToDate)
    {
        this->updateSemaphores();
        this->subOrderSemaphoresUpToDate = true;
    }

    std::vector<vk::Semaphore> semaphores = std::vector<vk::Semaphore>();

    // submit buffers which have a suborder first
    bool firstProcessed = false;
    const size_t lastGroupIndex = static_cast<size_t>(star::Command_Buffer_Order_Index::fifth) + 1;
    for (size_t i{1}; i < lastGroupIndex; i++)
    {
        if (!bufferGroupsWithSubOrders[order][i - 1].isInitialized())
        {
            continue;
        }

        if (shouldSubmitThisBuffer(bufferGroupsWithSubOrders[order][i - 1]))
        {
            CompleteRequest *buffer = this->allBuffers[bufferGroupsWithSubOrders[order][i - 1].getID()].get();

            if (buffer->beforeBufferSubmissionCallback.has_value())
                buffer->beforeBufferSubmissionCallback.value()(frameTracker.getCurrent().getFrameInFlightIndex());

            if (!buffer->recordOnce)
            {
                buffer->recordBufferCallback(*buffer->commandBuffer, frameTracker, currentFrameIndex);
            }

            vk::Semaphore doneSemaphore;
            if (!firstProcessed)
            {
                doneSemaphore = buffer->submitCommandBuffer(device, frameTracker, queues, additionalWaitSemaphores);
                firstProcessed = true;
            }
            else
            {
                doneSemaphore = buffer->submitCommandBuffer(device, frameTracker, queues);
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

void star::CommandBufferContainer::cleanup(core::device::StarDevice &device)
{
    for (auto &request : this->allBuffers)
    {
        request->commandBuffer->cleanupRender(device.getVulkanDevice());
    }
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
    for (int i = star::Command_Buffer_Order::before_render_pass; i != Command_Buffer_Order::end_of_frame; i++)
    {
        if (this->bufferGroupsWithSubOrders[static_cast<Command_Buffer_Order>(i)][0].isInitialized())
        {
            for (size_t j{0}; j < bufferGroupsWithSubOrders[static_cast<Command_Buffer_Order>(i)].size() - 1; j++)
            {
                auto &currentBuffer =
                    allBuffers[bufferGroupsWithSubOrders[static_cast<Command_Buffer_Order>(i)][j].getID()];
                const size_t nextIndex = j + 1;
                const auto &nextBufferHandle =
                    bufferGroupsWithSubOrders[static_cast<Command_Buffer_Order>(i)][nextIndex];

                if (nextIndex < bufferGroupsWithSubOrders.size() && nextBufferHandle.isInitialized())
                {
                    auto *nextBuffer = allBuffers[nextBufferHandle.getID()].get();
                    nextBuffer->commandBuffer->waitFor(currentBuffer->commandBuffer->getCompleteSemaphores(),
                                                       nextBuffer->waitStage);
                }
                else
                {
                    continue;
                }
            }
        }
    }
}