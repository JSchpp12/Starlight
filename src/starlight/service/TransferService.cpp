#include "starlight/service/TransferService.hpp"

#include "core/logging/LoggingFactory.hpp"
#include "starlight/core/Exceptions.hpp"

#include <star_common/HandleTypeRegistry.hpp>

#include <set>

namespace star::service
{
TransferService::TransferService(absl::flat_hash_map<star::Queue_Type, Handle> engineReservedQueues,
                                 size_t targetNumQueuesToUse)
    : m_engineReservedQueues(std::move(engineReservedQueues)), m_targetNumQueuesToUse(targetNumQueuesToUse)
{
}

TransferService::TransferService(absl::flat_hash_map<star::Queue_Type, Handle> engineReservedQueues,
                                 TransferServiceConfig config,
                                 size_t targetNumQueuesToUse)
    : m_engineReservedQueues(std::move(engineReservedQueues)),
      m_targetNumQueuesToUse(config.standardPriorityWorkerCount + 1),
      m_config(std::move(config))
{
    (void)targetNumQueuesToUse;
}

TransferService::TransferService(TransferService &&other)
    : m_device(other.m_device), m_graphicsManagers(other.m_graphicsManagers), m_eventBus(other.m_eventBus),
      m_taskManager(other.m_taskManager), m_workerPool(other.m_workerPool),
      m_engineReservedQueues(std::move(other.m_engineReservedQueues)),
      m_targetNumQueuesToUse(other.m_targetNumQueuesToUse),
      m_config(other.m_config)
{
}

TransferService &TransferService::operator=(TransferService &&other)
{
    if (this != &other)
    {
        m_device = other.m_device;
        m_graphicsManagers = other.m_graphicsManagers;
        m_eventBus = other.m_eventBus;
        m_taskManager = other.m_taskManager;
        m_workerPool = other.m_workerPool;
        m_engineReservedQueues = std::move(other.m_engineReservedQueues);
        m_targetNumQueuesToUse = other.m_targetNumQueuesToUse;
        m_config = other.m_config;
    }

    return *this;
}

void TransferService::setInitParameters(star::service::InitParameters &params)
{
    m_device = &params.device;
    m_graphicsManagers = &params.graphicsManagers;
    m_eventBus = &params.eventBus;
    m_taskManager = &params.taskManager;
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

    dispatchCreateWorkersFromConfig();
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

void TransferService::shutdown()
{
}
} // namespace star::service