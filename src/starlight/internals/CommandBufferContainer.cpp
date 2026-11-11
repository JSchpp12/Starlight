#include "internals/CommandBufferContainer.hpp"

#include "core/device/DeviceContext.hpp"
#include "core/graphics/GPUWorkSyncInfo.hpp"
#include "core/logging/LoggingFactory.hpp"
#include "starlight/setup/RenderResourceSetup.hpp"

#include <span>
#include <utility>

star::CommandBufferContainer::CommandBufferContainer(core::device::StarDevice &device, const uint8_t &numImagesInFlight)
    : bufferGroupsWithSubOrders({std::make_pair(star::Command_Buffer_Order::before_render_pass, std::vector<Handle>(5)),
                                 std::make_pair(star::Command_Buffer_Order::main_render_pass, std::vector<Handle>(5)),
                                 std::make_pair(star::Command_Buffer_Order::after_render_pass, std::vector<Handle>(5)),
                                 std::make_pair(star::Command_Buffer_Order::end_of_frame, std::vector<Handle>(5)),
                                 std::make_pair(star::Command_Buffer_Order::presentation, std::vector<Handle>(1))})
{
}

star::CommandBufferContainer::CompleteRequest::CompleteRequest(
    std::function<void(StarCommandBuffer &, const common::FrameTracker &, const uint64_t &)> recordBufferCallback,
    std::unique_ptr<StarCommandBuffer> commandBuffer, const Queue_Type &type, const bool &recordOnce,
    const vk::PipelineStageFlags &waitStage, const Command_Buffer_Order &order,
    std::optional<std::function<void(const int &)>> beforeSubmissionCallback,
    std::optional<StarCommandBuffer::SubmissionOverride> overrideBufferSubmissionCallback)
    : recordBufferCallback(recordBufferCallback), commandBuffer(std::move(commandBuffer)), type(type),
      recordOnce(recordOnce), waitStage(waitStage), order(order),
      beforeBufferSubmissionCallback(beforeSubmissionCallback),
      overrideBufferSubmissionCallback(overrideBufferSubmissionCallback)
{
};

vk::Semaphore star::CommandBufferContainer::CompleteRequest::submitCommandBuffer(
    core::device::StarDevice &device, const common::FrameTracker &frameTracker,
    absl::flat_hash_map<star::Queue_Type, StarQueue *> &queues, std::vector<vk::Semaphore> *beforeSemaphores)
{
    assert(cpuRecordingWaits.empty() && "CPU recording waits must run before the command buffer is submitted");

    auto &waits = scratch.data;
    waits.clear();
    oneTimeWaitSemaphoreInfo.giveMeOneTimeSemaphoreWaitInfo(waits);

    if (overrideBufferSubmissionCallback.has_value())
    {
        StarQueue *queue{queues[commandBuffer->getType()]};
        assert(queue != nullptr);

        return overrideBufferSubmissionCallback.value()(
            *commandBuffer, frameTracker, beforeSemaphores,
            std::span<const StarCommandBuffer::Data>{waits.data(), waits.size()}, *queue);
    }

    if (beforeSemaphores != nullptr && beforeSemaphores->front() != VK_NULL_HANDLE)
        waits.push_back(StarCommandBuffer::Data{beforeSemaphores->front(), waitStage, std::nullopt});

    commandBuffer->submit(frameTracker.getCurrent().getFrameInFlightIndex(),
                          queues[commandBuffer->getType()]->getVulkanQueue(),
                          std::span<const StarCommandBuffer::Data>{waits.data(), waits.size()});

    return commandBuffer->getCompleteSemaphores().at(frameTracker.getCurrent().getFrameInFlightIndex());
}

vk::Semaphore star::CommandBufferContainer::submitGroupWhenReady(
    core::device::StarDevice &device, const star::Command_Buffer_Order &order, const common::FrameTracker &frameTracker,
    const uint64_t &currentFrameIndex, absl::flat_hash_map<star::Queue_Type, StarQueue *> &queues,
    std::vector<vk::Semaphore> *additionalWaitSemaphores)
{
    if (!this->subOrderSemaphoresUpToDate)
    {
        this->updateSemaphores();
        this->subOrderSemaphoresUpToDate = true;
    }

    // submit buffers which have a suborder first
    bool firstProcessed = false;
    const size_t lastGroupIndex = static_cast<size_t>(star::Command_Buffer_Order_Index::fifth) + 1;
    vk::Semaphore lastInGroup = VK_NULL_HANDLE;
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

            const bool readinessOk = buffer->runCPURecordingWaits();
            if (!buffer->recordOnce)
            {
                if (readinessOk)
                    buffer->recordBufferCallback(*buffer->commandBuffer, frameTracker, currentFrameIndex);
                else
                    buffer->recordEmpty(static_cast<uint8_t>(frameTracker.getCurrent().getFrameInFlightIndex()));
            }

            vk::Semaphore result = VK_NULL_HANDLE;
            if (!firstProcessed)
            {
                result = buffer->submitCommandBuffer(device, frameTracker, queues, additionalWaitSemaphores);
                firstProcessed = true;
            }
            else
            {
                result = buffer->submitCommandBuffer(device, frameTracker, queues);
            }

            if (result != VK_NULL_HANDLE)
            {
                lastInGroup = result;
            }

            resetThisBufferStatus(bufferGroupsWithSubOrders[order][i - 1]);
        }
    }

    return lastInGroup;
}

star::Handle star::CommandBufferContainer::add(
    std::shared_ptr<star::CommandBufferContainer::CompleteRequest> newRequest, const bool &willBeSubmittedEachFrame,
    const star::Queue_Type &type, const star::Command_Buffer_Order &order,
    const star::Command_Buffer_Order_Index &subOrder)
{
    uint32_t count = 0;
    star::common::casts::SafeCast<size_t, uint32_t>(this->allBuffers.size(), count);

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

void star::CommandBufferContainer::CompleteRequest::add(setup::RenderResourceSetup::ResourcePrepDefinition definition)
{
    renderResourceSetup.add(std::move(definition));
}

bool star::CommandBufferContainer::CompleteRequest::runCPURecordingWaits()
{
    const std::vector<Handle> failed = cpuRecordingWaits.runAll();
    if (failed.empty())
        return true;

    // A failed transfer never signals its timeline, so drop its GPU wait. The pass still submits an empty
    // recording so group semaphores stay valid. Prepared definitions stay owned by the setup and keep their normal
    // lifecycle: each definition's own transfer was validated before it was added, so a failed waiter means the
    // pass-level waits are not ready this frame, not that a one-shot definition references a dead resource.
    for (const Handle &resource : failed)
        oneTimeWaitSemaphoreInfo.erase(resource);

    core::logging::error("CPU recording readiness failed for a transfer resource; skipping this pass's recording");
    return false;
}

void star::CommandBufferContainer::CompleteRequest::recordEmpty(uint8_t frameInFlightIndex)
{
    commandBuffer->begin(static_cast<int>(frameInFlightIndex));
    commandBuffer->buffer(frameInFlightIndex).end();
}

bool star::CommandBufferContainer::CompleteRequest::isDone() const
{
    return renderResourceSetup.isDone();
}

bool star::CommandBufferContainer::CompleteRequest::hasWork() const
{
    return renderResourceSetup.hasWork();
}

void star::CommandBufferContainer::CompleteRequest::prepareSetup(const core::device::DeviceContext &context)
{
    renderResourceSetup.prepare(context);
}

void star::CommandBufferContainer::CompleteRequest::recordSetup(vk::CommandBuffer commandBuffer,
                                                                const core::graphics::SemaphoreInfo &recordingGate)
{
    renderResourceSetup.record(commandBuffer, recordingGate);
}

void star::CommandBufferContainer::CompleteRequest::pollSetupReleases(core::device::DeviceContext &context)
{
    renderResourceSetup.pollReleases(context);
}

void star::CommandBufferContainer::CompleteRequest::cleanupSetup(core::device::DeviceContext &context)
{
    renderResourceSetup.cleanupRender(context);
}

star::CommandBufferContainer::CompleteRequest::~CompleteRequest()
{
    if (renderResourceSetup.hasWork())
    {
        core::logging::warning(
            "CompleteRequest destroyed with one-shot setup still pending; a phase cleanup was missed");
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