#include "device/managers/ManagerCommandBuffer.hpp"

#include "core/Exceptions.hpp"
#include "core/helper/queue/QueueHelpers.hpp"

#include <set>
#include <star_common/HandleTypeRegistry.hpp>

std::stack<star::Handle> star::core::device::manager::ManagerCommandBuffer::dynamicBuffersToSubmit =
    std::stack<star::Handle>();

namespace star::core::device::manager
{
static inline void GatherEngineDefaultQueueInfo(common::EventBus &eventBus,
                                                core::device::manager::CommandPool &commandPoolManager,
                                                core::device::manager::Queue &queueManager,
                                                const star::Queue_Type &type, StarCommandPool *&pool, StarQueue *&queue)
{
    Handle defaultQueue;

    {
        auto event =
            event::GetQueue::Builder().setQueueData(defaultQueue).getEngineDedicatedQueue().setQueueType(type).build();
        eventBus.emit(std::move(event));
    }

    if (!defaultQueue.isInitialized())
    {
        STAR_THROW("Unable to gather default engine info");
    }

    queue = &queueManager.get(defaultQueue)->queue;

    defaultQueue.type =
        common::HandleTypeRegistry::instance().getType(common::special_types::CommandPoolTypeName).value();

    pool = &commandPoolManager.get(defaultQueue)->commandPool;
}

} // namespace star::core::device::manager
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
        star::Queue_Type::Tcompute, getInUseInfoForType(star::Queue_Type::Tcompute)->queue));
    m_preparedQueues.insert(std::pair<star::Queue_Type, StarQueue *>(
        star::Queue_Type::Ttransfer, getInUseInfoForType(star::Queue_Type::Ttransfer)->queue));

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
                                                                        const common::FrameTracker &frameTracker)
{
    handleDynamicBufferRequests();

    return submitCommandBuffers(device, frameTracker, frameTracker.getCurrent().getGlobalFrameCounter());
}

vk::Semaphore star::core::device::manager::ManagerCommandBuffer::submitCommandBuffers(
    StarDevice &device, const common::FrameTracker &frameTracker, const uint64_t &currentFrameIndex)
{
    // determine the order of buffers to execute
    assert(this->mainGraphicsBufferHandle && "No main graphics buffer set -- not a valid rendering setup");

    // submit before
    std::vector<vk::Semaphore> beforeSemaphores = this->buffers.submitGroupWhenReady(
        device, Command_Buffer_Order::before_render_pass, frameTracker, currentFrameIndex, m_preparedQueues);

    // need to submit each group of buffers depending on the queue family they are in
    CommandBufferContainer::CompleteRequest &mainGraphicsBuffer = this->buffers.get(*this->mainGraphicsBufferHandle);

    if (mainGraphicsBuffer.beforeBufferSubmissionCallback.has_value())
        mainGraphicsBuffer.beforeBufferSubmissionCallback.value()(frameTracker.getCurrent().getFrameInFlightIndex());

    if (!mainGraphicsBuffer.recordOnce)
    {
        mainGraphicsBuffer.recordBufferCallback(*mainGraphicsBuffer.commandBuffer, frameTracker, currentFrameIndex);
    }

    auto mainGraphicsSemaphore =
        mainGraphicsBuffer.submitCommandBuffer(device, frameTracker, m_preparedQueues, &beforeSemaphores);

    assert(mainGraphicsSemaphore && "The main graphics complete semaphore is not valid. This might happen if the "
                                    "override function does not return a valid semaphore");

    std::vector<vk::Semaphore> waitSemaphores = {mainGraphicsSemaphore};

    std::vector<vk::Semaphore> finalSubmissionSemaphores = this->buffers.submitGroupWhenReady(
        device, Command_Buffer_Order::end_of_frame, frameTracker, currentFrameIndex, m_preparedQueues, &waitSemaphores);

    if (!finalSubmissionSemaphores.empty())
    {
        return finalSubmissionSemaphores[0];
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
