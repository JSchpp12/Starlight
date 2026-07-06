#include "starlight/service/TransferService.hpp"

#include "core/logging/LoggingFactory.hpp"
#include "starlight/core/Exceptions.hpp"

#include <star_common/HandleTypeRegistry.hpp>

#include <set>

namespace star::service
{
TransferService::TransferService(absl::flat_hash_map<star::Queue_Type, Handle> engineReservedQueues,
                                 size_t targetNumQueuesToUse)
    : m_engineReservedQueues(std::move(engineReservedQueues)), m_selectedQueues(),
      m_targetNumQueuesToUse(targetNumQueuesToUse), m_config(), m_listenerTransferTask(*this)
{
}

TransferService::TransferService(absl::flat_hash_map<star::Queue_Type, Handle> engineReservedQueues,
                                 TransferServiceConfig config, size_t targetNumQueuesToUse)
    : m_engineReservedQueues(std::move(engineReservedQueues)), m_selectedQueues(),
      m_targetNumQueuesToUse(config.standardPriorityWorkerCount + 1), m_config(std::move(config)),
      m_listenerTransferTask(*this)
{
    (void)targetNumQueuesToUse;
}

TransferService::TransferService(TransferService &&other) noexcept
    : m_workerQueueFamilyIndices(std::move(other.m_workerQueueFamilyIndices)),
      m_numStandardTransferWorkers(std::move(other.m_numStandardTransferWorkers)),
      m_nextStandardWorker(std::move(other.m_nextStandardWorker)),
      m_engineReservedQueues(std::move(other.m_engineReservedQueues)),
      m_selectedQueues(std::move(other.m_selectedQueues)), m_targetNumQueuesToUse(other.m_targetNumQueuesToUse),
      m_config(other.m_config), m_listenerTransferTask(*this), m_device(other.m_device),
      m_graphicsManagers(other.m_graphicsManagers), m_eventBus(other.m_eventBus), m_cmdBus(std::move(other.m_cmdBus)),
      m_taskManager(other.m_taskManager), m_workerPool(other.m_workerPool)
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
    initListeners(*m_cmdBus);
}

void TransferService::onSubmitTransfer(star::command::transfer::SubmitTransferTask &cmd)
{
    using namespace job::tasks::transfer;
    assert(m_taskManager && "TransferService not initialized");

    auto *payload = static_cast<TransferPayload *>(cmd.task.getPayload());
    const bool high = (payload->priority == TransferPriority::High);

    // High priority -> dedicated worker 0.
    if (high)
    {
        m_taskManager->submitTask(std::move(cmd.task), transferWorkerHandle(0));
        cmd.getReply().set({.queueFamilyIndex = m_workerQueueFamilyIndices[0], .workerId = uint16_t{0}});
        return;
    }

    // Standard: round-robin across 1..N (or worker 0 if only one worker exists).
    const uint16_t initialWorkerId =
        m_numStandardTransferWorkers == 1
            ? uint16_t{0}
            : static_cast<uint16_t>(
                  (m_nextStandardWorker++ % m_numStandardTransferWorkers) + 1);

    auto tryWorker = [&](uint16_t id) -> bool {
        const Handle h = transferWorkerHandle(id);
        if (!m_taskManager->getIsWorkerQueueFull(cmd.task, h))
        {
            m_taskManager->submitTask(std::move(cmd.task), h);
            cmd.getReply().set({
                .queueFamilyIndex = m_workerQueueFamilyIndices[id],
                .workerId = id,
            });
            return true;
        }
        return false;
    };

    if (tryWorker(initialWorkerId))
        return;

    if (m_numStandardTransferWorkers > 1)
    {
        for (size_t i = 1; i <= m_numStandardTransferWorkers; ++i)
        {
            const uint16_t candidate = static_cast<uint16_t>(i);
            if (candidate == initialWorkerId)
                continue;
            if (tryWorker(candidate))
                return;
        }
    }

    // All standard workers full -> fall back to worker 0 (blocks there; its loop drains HP first).
    m_taskManager->submitTask(std::move(cmd.task), transferWorkerHandle(uint16_t{0}));
    cmd.getReply().set({
        .queueFamilyIndex = m_workerQueueFamilyIndices[0],
        .workerId = uint16_t{0},
    });
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
}

void TransferService::initListeners(star::core::CommandBus &cmdBus)
{
    m_listenerTransferTask.init(cmdBus);
}

Handle TransferService::transferWorkerHandle(uint16_t workerIndex) const noexcept
{
    return Handle{
        .type = common::HandleTypeRegistry::instance().getTypeGuaranteedExist(job::tasks::transfer::TransferTaskName),
        .id = workerIndex};
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