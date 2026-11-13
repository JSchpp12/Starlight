#include "device/managers/ManagerCommandBuffer.hpp"

#include "core/Exceptions.hpp"
#include "core/helper/queue/QueueHelpers.hpp"
#include "starlight/event/PassCPURecordingWaitResolved.hpp"

#include <exception>
#include <set>
#include <star_common/EventBus.hpp>
#include <star_common/HandleTypeRegistry.hpp>

std::stack<star::Handle> star::core::device::manager::ManagerCommandBuffer::dynamicBuffersToSubmit =
    std::stack<star::Handle>();

star::core::device::manager::ManagerCommandBuffer::ManagerCommandBuffer(
    StarDevice &device, core::device::manager::Queue &queueManager, const uint8_t &numFramesInFlight,
    const absl::flat_hash_map<star::Queue_Type, Handle> &queuesToUse)
    : numFramesInFlight(numFramesInFlight), buffers(device, numFramesInFlight)
{
    for (auto &ele : queuesToUse)
    {
        m_typeToQueueInfo.insert(std::pair<star::Queue_Type, Handle>(ele.first, ele.second));

        if (!m_inUseQueueInfo.contains(ele.second))
        {
            StarCommandPool pool{device.getVulkanDevice(),
                                 queueManager.get(ele.second)->queue.getParentQueueFamilyIndex(), true};

            m_inUseQueueInfo.insert(std::pair<Handle, InUseQueueInfo>(ele.second, InUseQueueInfo{std::move(pool)}));
        }
    }
}

void star::core::device::manager::ManagerCommandBuffer::cleanup(StarDevice &device)
{
    buffers.cleanup(device);

    std::set<StarCommandPool *> pools;
    for (auto &ele : m_inUseQueueInfo)
    {
        pools.insert(&ele.second.pool);
    }

    for (auto &pool : pools)
    {
        pool->cleanupRender(device.getVulkanDevice());
    }

    m_inUseQueueInfo.clear();
    m_typeToQueueInfo.clear();
}

void star::core::device::manager::ManagerCommandBuffer::init(core::device::manager::Queue &queueManager)
{
    // populate inUseQueueInfo
    for (auto &ele : m_inUseQueueInfo)
    {
        StarQueue &queue = queueManager.get(ele.first)->queue;
        ele.second.queue = &queue;
    }

    m_preparedQueues.clear();
    m_preparedQueues.insert(std::pair<star::Queue_Type, StarQueue *>(
        star::Queue_Type::Tgraphics, getInUseInfoForType(star::Queue_Type::Tgraphics)->queue));
    m_preparedQueues.insert(std::pair<star::Queue_Type, StarQueue *>(
        star::Queue_Type::Ttransfer, getInUseInfoForType(star::Queue_Type::Tgraphics)->queue));
    m_preparedQueues.insert(std::pair<star::Queue_Type, StarQueue *>(
        star::Queue_Type::Tcompute, getInUseInfoForType(star::Queue_Type::Tcompute)->queue));

    auto *info = getInUseInfoForType(star::Queue_Type::Tpresent);
    if (info != nullptr)
    {
        m_preparedQueues.insert(std::pair<star::Queue_Type, StarQueue *>(star::Queue_Type::Tpresent, info->queue));
    }
}

star::Handle star::core::device::manager::ManagerCommandBuffer::submit(StarDevice &device,
                                                                       const uint64_t &currentFrameIndex,
                                                                       Request request)
{
    // need to get a command pool for the type
    auto *target = selectQueueForType(request.type);
    if (target == nullptr)
    {
        STAR_THROW("Invalid type of queue requested from manager");
    }

    StarCommandPool *targetPool = &target->pool;

    assert(targetPool != nullptr);

    star::Handle newHandle =
        this->buffers.add(std::make_shared<CommandBufferContainer::CompleteRequest>(
                              request.recordBufferCallback,
                              std::make_unique<StarCommandBuffer>(
                                  device.getVulkanDevice(), this->numFramesInFlight, targetPool, request.type,
                                  !request.overrideBufferSubmissionCallback.has_value(), true),
                              request.type, request.recordOnce, request.waitStage, request.order,
                              request.beforeBufferSubmissionCallback, request.overrideBufferSubmissionCallback),
                          request.willBeSubmittedEachFrame, request.type, request.order,
                          static_cast<Command_Buffer_Order_Index>(request.orderIndex));

    if (request.type == Queue_Type::Tgraphics && request.order == Command_Buffer_Order::main_render_pass)
        this->mainGraphicsBufferHandle = std::make_unique<Handle>(newHandle);

    // todo: REMOVE THIS! OR move it to the update function
    if (request.recordOnce)
    {
        STAR_THROW("Single time recording is not supported. This feature is to be removed.");
    }
    return newHandle;
}

void star::core::device::manager::ManagerCommandBuffer::submitDynamicBuffer(Handle bufferHandle)
{
    ManagerCommandBuffer::dynamicBuffersToSubmit.push(bufferHandle);
}

star::CommandBufferContainer::CompleteRequest &star::core::device::manager::ManagerCommandBuffer::get(
    const Handle &handle)
{
    return buffers.get(handle);
}

star::CommandBufferContainer::CompleteRequest &star::core::device::manager::ManagerCommandBuffer::getDefault()
{
    assert(this->mainGraphicsBufferHandle);
    return this->buffers.get(*this->mainGraphicsBufferHandle);
}

vk::Semaphore star::core::device::manager::ManagerCommandBuffer::update(StarDevice &device,
                                                                        const common::FrameTracker &frameTracker,
                                                                        common::EventBus &eventBus)
{
    handleDynamicBufferRequests();

    std::vector<CommandBufferContainer::WaitOutcome> outcomes;
    const vk::Semaphore result =
        submitCommandBuffers(device, frameTracker, frameTracker.getCurrent().getGlobalFrameCounter(), outcomes);

    // Emit one event after all submissions so handlers never mutate command-buffer groups mid-iteration. The owning
    // service applies the failure policy (for example, MaterialService terminates on its material readiness
    // conditions).
    if (!outcomes.empty())
    {
        star::event::PassCPURecordingWaitResolved::Results results;
        results.reserve(outcomes.size());

        for (const auto &outcome : outcomes)
        {
            results.push_back(star::event::PassWaitResultInfo{.pass = outcome.pass,
                                                              .failedResources = outcome.failedResources,
                                                              .pendingResources = outcome.pendingResources,
                                                              .onFailure = outcome.policy});
        }

        eventBus.emit(star::event::PassCPURecordingWaitResolved{std::move(results)});
    }

    return result;
}

vk::Semaphore star::core::device::manager::ManagerCommandBuffer::submitCommandBuffers(
    StarDevice &device, const common::FrameTracker &frameTracker, const uint64_t &currentFrameIndex,
    std::vector<CommandBufferContainer::WaitOutcome> &outOutcomes)
{
    // determine the order of buffers to execute
    assert(this->mainGraphicsBufferHandle && "No main graphics buffer set -- not a valid rendering setup");

    // submit before
    std::vector<vk::Semaphore> beforeSemaphores = {
        this->buffers.submitGroupWhenReady(device, Command_Buffer_Order::before_render_pass, frameTracker,
                                           currentFrameIndex, m_preparedQueues, nullptr, &outOutcomes)};

    // need to submit each group of buffers depending on the queue family they are in
    CommandBufferContainer::CompleteRequest &mainGraphicsBuffer = this->buffers.get(*this->mainGraphicsBufferHandle);

    if (mainGraphicsBuffer.beforeBufferSubmissionCallback.has_value())
        mainGraphicsBuffer.beforeBufferSubmissionCallback.value()(frameTracker.getCurrent().getFrameInFlightIndex());

    if (!mainGraphicsBuffer.recordOnce)
    {
        mainGraphicsBuffer.recordBufferCallback(*mainGraphicsBuffer.commandBuffer, frameTracker, currentFrameIndex);
    }

    const auto waitResult = mainGraphicsBuffer.runCPURecordingWaits();
    if (waitResult.hadWaits)
    {
        outOutcomes.push_back(CommandBufferContainer::WaitOutcome{.pass = *this->mainGraphicsBufferHandle,
                                                                  .failedResources = waitResult.failed,
                                                                  .pendingResources = waitResult.pending,
                                                                  .policy = waitResult.policy});
    }
    auto mainGraphicsSemaphore =
        mainGraphicsBuffer.submitCommandBuffer(device, frameTracker, m_preparedQueues, &beforeSemaphores);

    assert(mainGraphicsSemaphore && "The main graphics complete semaphore is not valid. This might happen if the "
                                    "override function does not return a valid semaphore");

    std::vector<vk::Semaphore> waitSemaphores = {mainGraphicsSemaphore};

    // Frame-wide transfer drain: wait for every registered transfer to be CPU-published immediately before the
    // finalization phase records. The preceding before-render submits are timeline-only, so this is the first point
    // where the binary-semaphore dependency rule requires the transfer signals to be submitted.
    const auto frameWaitResult = m_beforeFinalizationWaits.runAll();
    if (frameWaitResult.hadWaits)
    {
        outOutcomes.push_back(CommandBufferContainer::WaitOutcome{.pass = Handle{},
                                                                  .failedResources = frameWaitResult.failed,
                                                                  .pendingResources = frameWaitResult.pending,
                                                                  .policy = frameWaitResult.policy});
    }
    vk::Semaphore finalSubmissionSemaphores =
        this->buffers.submitGroupWhenReady(device, Command_Buffer_Order::end_of_frame, frameTracker, currentFrameIndex,
                                           m_preparedQueues, &waitSemaphores, &outOutcomes);

    if (finalSubmissionSemaphores != VK_NULL_HANDLE)
    {
        return finalSubmissionSemaphores;
    }
    return mainGraphicsSemaphore;
}

void star::core::device::manager::ManagerCommandBuffer::handleDynamicBufferRequests()
{
    while (!ManagerCommandBuffer::dynamicBuffersToSubmit.empty())
    {
        Handle dynamicBufferRequest = ManagerCommandBuffer::dynamicBuffersToSubmit.top();

        this->buffers.setToSubmitThisBuffer(dynamicBufferRequest);

        ManagerCommandBuffer::dynamicBuffersToSubmit.pop();
    }
}

namespace star::core::device::manager
{
const ManagerCommandBuffer::InUseQueueInfo *ManagerCommandBuffer::getInUseInfoForType(const star::Queue_Type &type)
{
    return selectQueueForType(type);
}

ManagerCommandBuffer::InUseQueueInfo *ManagerCommandBuffer::selectQueueForType(const star::Queue_Type &type)
{
    Handle *target = &m_typeToQueueInfo.at(type);
    if (target != nullptr)
    {
        return &m_inUseQueueInfo.at(*target);
    }

    return nullptr;
}
} // namespace star::core::device::manager
