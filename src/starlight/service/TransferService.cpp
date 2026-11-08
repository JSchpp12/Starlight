#include "starlight/service/TransferService.hpp"

#include "core/logging/LoggingFactory.hpp"
#include "starlight/core/Exceptions.hpp"

#include <star_common/HandleTypeRegistry.hpp>

#include <future>
#include <set>
#include <variant>

namespace star::service
{
TransferService::TransferService(absl::flat_hash_map<star::Queue_Type, Handle> engineReservedQueues,
                                 size_t targetNumQueuesToUse)
    : m_engineReservedQueues(std::move(engineReservedQueues)), m_selectedQueues(),
      m_targetNumQueuesToUse(targetNumQueuesToUse), m_config(), m_listenerTransferTask(*this),
      m_listenerTextureTransfer(*this), m_listenerBufferTransfer(*this)
{
}

TransferService::TransferService(absl::flat_hash_map<star::Queue_Type, Handle> engineReservedQueues,
                                 TransferServiceConfig config, size_t targetNumQueuesToUse)
    : m_engineReservedQueues(std::move(engineReservedQueues)), m_selectedQueues(),
      m_targetNumQueuesToUse(config.standardPriorityWorkerCount + 1), m_config(std::move(config)),
      m_listenerTransferTask(*this), m_listenerTextureTransfer(*this), m_listenerBufferTransfer(*this)
{
    (void)targetNumQueuesToUse;
}

TransferService::TransferService(TransferService &&other) noexcept
    : m_workerSemaphores(std::move(other.m_workerSemaphores)),
      m_workerQueueFamilyIndices(std::move(other.m_workerQueueFamilyIndices)),
      m_numStandardTransferWorkers(std::move(other.m_numStandardTransferWorkers)),
      m_nextStandardWorker(std::move(other.m_nextStandardWorker)),
      m_engineReservedQueues(std::move(other.m_engineReservedQueues)),
      m_selectedQueues(std::move(other.m_selectedQueues)), m_targetNumQueuesToUse(other.m_targetNumQueuesToUse),
      m_config(other.m_config), m_listenerTransferTask(*this), m_listenerTextureTransfer(*this),
      m_listenerBufferTransfer(*this), m_device(other.m_device), m_graphicsManagers(other.m_graphicsManagers),
      m_eventBus(other.m_eventBus), m_cmdBus(std::move(other.m_cmdBus)), m_taskManager(other.m_taskManager),
      m_workerPool(other.m_workerPool)
{
    if (m_cmdBus != nullptr)
    {
        other.cleanupListeners(*m_cmdBus);
        initListeners(*m_cmdBus);
    }
}

TransferService &TransferService::operator=(TransferService &&other) noexcept
{
    if (this != &other)
    {
        m_workerSemaphores = std::move(other.m_workerSemaphores);

        m_workerQueueFamilyIndices = std::move(other.m_workerQueueFamilyIndices);
        m_numStandardTransferWorkers = std::move(other.m_numStandardTransferWorkers);
        m_nextStandardWorker = std::move(other.m_nextStandardWorker);

        m_device = other.m_device;
        m_graphicsManagers = other.m_graphicsManagers;
        m_eventBus = other.m_eventBus;
        m_cmdBus = std::move(other.m_cmdBus);
        m_taskManager = other.m_taskManager;
        m_workerPool = other.m_workerPool;
        m_engineReservedQueues = std::move(other.m_engineReservedQueues);
        m_selectedQueues = std::move(other.m_selectedQueues);
        m_targetNumQueuesToUse = other.m_targetNumQueuesToUse;
        m_config = other.m_config;

        if (m_cmdBus != nullptr)
        {
            other.cleanupListeners(*m_cmdBus);
            initListeners(*m_cmdBus);
        }
    }

    return *this;
}

void TransferService::setInitParameters(star::service::InitParameters &params)
{
    m_device = &params.device;
    m_graphicsManagers = &params.graphicsManagers;
    m_eventBus = &params.eventBus;
    m_taskManager = &params.taskManager;
    m_cmdBus = &params.commandBus;
}

void TransferService::negotiateWorkers(core::WorkerPool &pool, job::TaskManager &tm)
{
    m_workerPool = &pool;
    m_taskManager = &tm;
}

void TransferService::init()
{
    assert(m_device != nullptr && "Device not saved from initParameters");
    assert(m_graphicsManagers != nullptr && "Graphics managers not saved from initParameters");
    assert(m_eventBus != nullptr && "Event bus not saved from initParameters");
    assert(m_taskManager != nullptr && "Task manager not saved from initParameters");
    assert(m_cmdBus != nullptr);

    selectQueueFamiliesToUse();
    dispatchCreateWorkersFromConfig();
    initSemaphores();
    initListeners(*m_cmdBus);
}

star::command::transfer::SubmitTransferResult TransferService::submitTransferTask(
    job::tasks::transfer::TransferTask task, job::tasks::transfer::TransferPriority priority)
{
    using namespace job::tasks::transfer;
    assert(m_taskManager && "TransferService not initialized");

    auto *payload = static_cast<TransferPayload *>(task.getPayload());
    const bool high = (priority == TransferPriority::High);

    // A request that keeps its staging buffer alive gets a buffer-manager slot registered here, on the submitting
    // thread, before the task reaches a worker. The worker fills the promise once the buffer exists; the returned
    // handle travels to the caller, which releases it after the recorded work on it has completed.
    std::optional<Handle> stagingBuffer{};
    if (auto *textureData = std::get_if<job::TransferManagerThread::TextureTransferData>(&payload->request->data);
        textureData != nullptr && textureData->request->takesStagingOwnership)
    {
        auto stagingPromise = std::promise<core::device::manager::PendingResult<star::StarBuffers::Buffer>>{};
        stagingBuffer =
            m_graphicsManagers->bufferManager.submit(core::device::manager::BufferRequest{stagingPromise.get_future()});
        textureData->request->stagingBufferSlot = std::move(stagingPromise);
    }

    auto applySync = [&](uint16_t id) -> star::core::graphics::SemaphoreInfo {
        StarSemaphore *s =
            (id == 0) ? (high ? &m_workerSemaphores[1] : &m_workerSemaphores[0]) : &m_workerSemaphores[id + 1];
        ++s->signalValue;
        payload->request->workSyncInfo.workSignalWhenDone = {.signalValue = s->signalValue,
                                                             .semaphore = s->vkSemaphore};
        return star::core::graphics::SemaphoreInfo{.signalValue = s->signalValue, .semaphore = s->vkSemaphore};
    };

    // High priority -> dedicated worker 0.
    if (high)
    {
        auto sync = applySync(0);

        m_taskManager->submitTask(std::move(task), transferWorkerHandle(0));
        return star::command::transfer::SubmitTransferResult{
            .semaphore = {.vkSemaphore = sync.semaphore, .signalValue = sync.signalValue},
            .queueFamilyIndex = m_workerQueueFamilyIndices[0],
            .workerId = uint16_t{0},
            .stagingBuffer = stagingBuffer};
    }

    // Standard: round-robin across 1..N (or worker 0 if only one worker exists).
    const uint16_t initialWorkerId =
        m_numStandardTransferWorkers == 1
            ? uint16_t{0}
            : static_cast<uint16_t>((m_nextStandardWorker++ % m_numStandardTransferWorkers) + 1);

    auto tryWorker = [&](uint16_t id) -> std::optional<star::command::transfer::SubmitTransferResult> {
        const Handle h = transferWorkerHandle(id);
        if (!m_taskManager->getIsWorkerQueueFull(task, h))
        {
            auto sync = applySync(id);
            m_taskManager->submitTask(std::move(task), h);
            return star::command::transfer::SubmitTransferResult{.semaphore =
                                                                     StarSemaphore{sync.semaphore, sync.signalValue},
                                                                 .queueFamilyIndex = m_workerQueueFamilyIndices[id],
                                                                 .workerId = id,
                                                                 .stagingBuffer = stagingBuffer};
        }
        return std::nullopt;
    };

    if (auto result = tryWorker(initialWorkerId))
        return *result;

    if (m_numStandardTransferWorkers > 1)
    {
        for (size_t i = 1; i <= m_numStandardTransferWorkers; ++i)
        {
            const uint16_t candidate = static_cast<uint16_t>(i);
            if (candidate == initialWorkerId)
                continue;
            if (auto result = tryWorker(candidate))
                return *result;
        }
    }

    // All standard workers full -> fall back to worker 0 (blocks there; its loop drains HP first).
    auto sync = applySync(0);
    m_taskManager->submitTask(std::move(task), transferWorkerHandle(uint16_t{0}));
    return star::command::transfer::SubmitTransferResult{
        .semaphore = StarSemaphore{.vkSemaphore = sync.semaphore, .signalValue = sync.signalValue},
        .queueFamilyIndex = m_workerQueueFamilyIndices[0],
        .workerId = uint16_t{0},
        .stagingBuffer = stagingBuffer};
}

void TransferService::onSubmitTransfer(star::command::transfer::SubmitTransferTask &cmd)
{
    auto *payload = static_cast<job::tasks::transfer::TransferPayload *>(cmd.task.getPayload());
    const auto priority = payload->priority;

    cmd.getReply().set(submitTransferTask(std::move(cmd.task), priority));
}

void TransferService::onSubmitTexture(star::command::transfer::SubmitTextureTransfer &cmd)
{
    using namespace job::tasks::transfer;

    assert(m_graphicsManagers && m_taskManager && "TransferService not initialized");

    // The Image manager owns the texture from here on: its record holds a future the transfer worker fills with the
    // created texture (or the exception that explains why it failed). No ManagerRenderResource record is allocated for
    // it.
    std::promise<core::device::manager::PendingResult<star::StarTextures::Texture>> texturePromise{};
    const Handle image =
        m_graphicsManagers->imageManager.submit(core::device::manager::ImageRequest{texturePromise.get_future()});

    auto interThreadRequest = std::make_unique<job::TransferManagerThread::InterThreadRequest>(
        nullptr, std::move(cmd.request), std::move(texturePromise));
    const auto result = submitTransferTask(
        CreateTransferTask(TransferPayload{TransferPriority::Standard, std::move(interThreadRequest)}),
        TransferPriority::Standard);

    cmd.getReply().set({.image = image, .semaphore = result.semaphore, .stagingBuffer = result.stagingBuffer});
}

void TransferService::onSubmitBuffer(star::command::transfer::SubmitBufferTransfer &cmd)
{
    using namespace job::tasks::transfer;

    assert(m_graphicsManagers && m_taskManager && "TransferService not initialized");

    // The Buffer manager owns the buffer from here on: its record holds a future the transfer worker fills with the
    // created buffer (or the exception that explains why it failed). No ManagerRenderResource record is allocated for
    // it.
    std::promise<core::device::manager::PendingResult<star::StarBuffers::Buffer>> bufferPromise{};
    const Handle buffer =
        m_graphicsManagers->bufferManager.submit(core::device::manager::BufferRequest{bufferPromise.get_future()});

    auto interThreadRequest = std::make_unique<job::TransferManagerThread::InterThreadRequest>(
        nullptr, std::move(cmd.request), std::move(bufferPromise));
    const auto result = submitTransferTask(
        CreateTransferTask(TransferPayload{TransferPriority::Standard, std::move(interThreadRequest)}),
        TransferPriority::Standard);

    cmd.getReply().set({.buffer = buffer, .semaphore = result.semaphore});
}

void TransferService::dispatchCreateWorkersFromConfig()
{
    const auto hi = m_config.highPrioritySize;
    const auto std = m_config.standardPrioritySize;

    switch (hi)
    {
    case star::TransferQueueCapacity::Low:
        switch (std)
        {
        case star::TransferQueueCapacity::Low:
            return dispatchCreateWorkers<64, 64>();
        case star::TransferQueueCapacity::Medium:
            return dispatchCreateWorkers<64, 128>();
        case star::TransferQueueCapacity::High:
            return dispatchCreateWorkers<64, 256>();
        case star::TransferQueueCapacity::Ultra:
            return dispatchCreateWorkers<64, 512>();
        }
        break;
    case star::TransferQueueCapacity::Medium:
        switch (std)
        {
        case star::TransferQueueCapacity::Low:
            return dispatchCreateWorkers<128, 64>();
        case star::TransferQueueCapacity::Medium:
            return dispatchCreateWorkers<128, 128>();
        case star::TransferQueueCapacity::High:
            return dispatchCreateWorkers<128, 256>();
        case star::TransferQueueCapacity::Ultra:
            return dispatchCreateWorkers<128, 512>();
        }
        break;
    case star::TransferQueueCapacity::High:
        switch (std)
        {
        case star::TransferQueueCapacity::Low:
            return dispatchCreateWorkers<256, 64>();
        case star::TransferQueueCapacity::Medium:
            return dispatchCreateWorkers<256, 128>();
        case star::TransferQueueCapacity::High:
            return dispatchCreateWorkers<256, 256>();
        case star::TransferQueueCapacity::Ultra:
            return dispatchCreateWorkers<256, 512>();
        }
        break;
    case star::TransferQueueCapacity::Ultra:
        switch (std)
        {
        case star::TransferQueueCapacity::Low:
            return dispatchCreateWorkers<512, 64>();
        case star::TransferQueueCapacity::Medium:
            return dispatchCreateWorkers<512, 128>();
        case star::TransferQueueCapacity::High:
            return dispatchCreateWorkers<512, 256>();
        case star::TransferQueueCapacity::Ultra:
            return dispatchCreateWorkers<512, 512>();
        }
        break;
    }
}

void TransferService::cleanupListeners(star::core::CommandBus &cmdBus)
{
    m_listenerTransferTask.cleanup(cmdBus);
    m_listenerTextureTransfer.cleanup(cmdBus);
    m_listenerBufferTransfer.cleanup(cmdBus);
}

void TransferService::initListeners(star::core::CommandBus &cmdBus)
{
    m_listenerTransferTask.init(cmdBus);
    m_listenerTextureTransfer.init(cmdBus);
    m_listenerBufferTransfer.init(cmdBus);
}

Handle TransferService::transferWorkerHandle(uint16_t workerIndex) const noexcept
{
    return Handle{
        .type = common::HandleTypeRegistry::instance().getTypeGuaranteedExist(job::tasks::transfer::TransferTaskName),
        .id = workerIndex};
}

void TransferService::initSemaphores()
{
    // One semaphore per worker plus one: worker 0 uses [0] for standard and [1] for
    // high-priority submissions; worker N uses [N + 1] (see applySync in onSubmitTransfer).
    const size_t numWorkers = m_taskManager->getNumOfWorkersForType(transferWorkerHandle(0));
    m_workerSemaphores.resize(numWorkers + 1);

    for (size_t i{0}; i < m_workerSemaphores.size(); i++)
    {
        const auto nS =
            m_graphicsManagers->semaphoreManager->submit(star::core::device::manager::SemaphoreRequest(uint64_t{0}));
        const auto *r = m_graphicsManagers->semaphoreManager->get(nS);
        m_workerSemaphores[i] =
            star::StarSemaphore{.vkSemaphore = r->semaphore, .signalValue = r->timelineValue.value()};
    }
}

void TransferService::shutdown()
{
    if (m_cmdBus != nullptr)
        cleanupListeners(*m_cmdBus);
}

void TransferService::selectQueueFamiliesToUse()
{
    std::unordered_set<uint8_t> engineQueues;
    // get the queue families to avoid we want a dedicated one for the transfer operations
    for (uint8_t i{0}; i < star::Queue_Type::Tpresent; ++i)
    {
        star::Handle eQ;
        const star::Queue_Type type = static_cast<star::Queue_Type>(i);

        m_eventBus->emit(
            star::event::GetQueue::Builder().setQueueData(eQ).setQueueType(type).getEngineDedicatedQueue().build());

        if (eQ.isInitialized())
        {
            auto result = m_graphicsManagers->queueManager.get(eQ);
            engineQueues.insert(result->queue.getParentQueueFamilyIndex());
        }
    }

    m_selectedQueues.reserve(m_targetNumQueuesToUse);

    while (m_selectedQueues.size() < m_targetNumQueuesToUse)
    {
        star::Handle queue;
        // select a queue family to use
        m_eventBus->emit(star::event::GetQueue::Builder()
                             .setQueueData(queue)
                             .setQueueType(star::Queue_Type::Ttransfer)
                             .setAvoidFamilyIndex(engineQueues)
                             .build());

        if (!queue.isInitialized())
        {
            if (m_selectedQueues.size() <= 1)
            {
                star::core::logging::warning(
                    "Rendering device does not have dedicated transfer queue available. Transfer "
                    "service will revert to use default engine queue");
                STAR_THROW("Fallback to using default engine queue not implemented");
            }
            else
            {
                break;
            }
        }

        // drain the family
        const auto selectedIndex =
            static_cast<uint8_t>(m_graphicsManagers->queueManager.get(queue)->queue.getParentQueueFamilyIndex());
        while (queue.isInitialized() && m_selectedQueues.size() < m_targetNumQueuesToUse)
        {
            m_selectedQueues.push_back(queue);
            m_eventBus->emit(star::event::GetQueue::Builder()
                                 .setQueueData(queue)
                                 .setQueueType(star::Queue_Type::Ttransfer)
                                 .setSelectFromFamilyIndex(std::unordered_set<uint8_t>{selectedIndex})
                                 .build());
        }
    }
}
} // namespace star::service