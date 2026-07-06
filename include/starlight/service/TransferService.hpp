#pragma once

#include "InitParameters.hpp"
#include "TransferServiceConfig.hpp"
#include "starlight/command/transfer/SubmitTransferTask.hpp"
#include "starlight/core/WorkerPool.hpp"
#include "starlight/core/device/StarDevice.hpp"
#include "starlight/core/device/managers/GraphicsContainer.hpp"
#include "starlight/event/GetQueue.hpp"
#include "starlight/job/TaskManager.hpp"
#include "starlight/job/worker/DefaultWorker.hpp"
#include "starlight/job/worker/Worker.hpp"
#include "starlight/job/worker/detail/default_worker/BusyWaitTransferTaskHandlingPolicy.hpp"
#include "starlight/managers/ManagerRenderResource.hpp"
#include "starlight/policy/command/ListenFor.hpp"

#include <star_common/EventBus.hpp>

#include <absl/container/flat_hash_map.h>

#include <sstream>

namespace star::service
{
template <typename T>
using ListenForSubmitTransferTask =
    star::policy::command::ListenFor<T, star::command::transfer::SubmitTransferTask,
                                     star::command::transfer::submit_transfer::GetUniqueTypeName, &T::onSubmitTransfer>;

class TransferService
{
  public:
    TransferService() = default;
    explicit TransferService(absl::flat_hash_map<star::Queue_Type, Handle> engineReservedQueues,
                             size_t targetNumQueuesToUse = 1);
    explicit TransferService(absl::flat_hash_map<star::Queue_Type, Handle> engineReservedQueues,
                             TransferServiceConfig config, size_t targetNumQueuesToUse = 1);
    TransferService(const TransferService &) = delete;
    TransferService &operator=(const TransferService &) = delete;
    TransferService(TransferService &&other) noexcept;
    TransferService &operator=(TransferService &&other) noexcept;
    ~TransferService() = default;

    void init();

    void negotiateWorkers(core::WorkerPool &pool, job::TaskManager &tm);

    void setInitParameters(star::service::InitParameters &params);

    void shutdown();

    void onSubmitTransfer(star::command::transfer::SubmitTransferTask &cmd);

  private:
    std::vector<uint32_t> m_workerQueueFamilyIndices;
    size_t m_numStandardTransferWorkers = 0;
    size_t m_nextStandardWorker{0};
    absl::flat_hash_map<star::Queue_Type, Handle> m_engineReservedQueues;
    std::vector<Handle> m_selectedQueues;
    size_t m_targetNumQueuesToUse{1};
    TransferServiceConfig m_config{};
    ListenForSubmitTransferTask<TransferService> m_listenerTransferTask;
    core::device::StarDevice *m_device{nullptr};
    core::device::manager::GraphicsContainer *m_graphicsManagers{nullptr};
    common::EventBus *m_eventBus{nullptr};
    star::core::CommandBus *m_cmdBus{nullptr};
    job::TaskManager *m_taskManager{nullptr};
    core::WorkerPool *m_workerPool{nullptr};

    void selectQueueFamiliesToUse();

    template <typename THighPriorityWorkerPolicy, typename TStandardPriorityWorkerPolicy>
    void createAndRegisterTransferWorkers()
    {
        core::logging::log(boost::log::trivial::info, "Initializing transfer workers");

        std::vector<StarQueue *> transferWorkerQueues;
        transferWorkerQueues.reserve(m_selectedQueues.size());

        for (const auto &q : m_selectedQueues)
        {
            transferWorkerQueues.push_back(&m_graphicsManagers->queueManager.get(q)->queue);
        }

        if (m_selectedQueues.size() < m_targetNumQueuesToUse)
        {
            std::ostringstream queueOss;
            queueOss << "Requested " << m_targetNumQueuesToUse << " transfer queues, obtained "
                     << m_selectedQueues.size() << ". Some standard workers will not be created.";
            core::logging::log(boost::log::trivial::warning, queueOss.str());
        }

        std::set<uint32_t> uniqueQueueFamilyIndicesInUse;
        for (const auto *queue : transferWorkerQueues)
        {
            if (queue != nullptr)
                uniqueQueueFamilyIndicesInUse.insert(queue->getParentQueueFamilyIndex());
        }

        std::vector<uint32_t> allTransferQueueFamilyIndicesInUse;
        for (const auto &index : uniqueQueueFamilyIndicesInUse)
        {
            allTransferQueueFamilyIndicesInUse.push_back(index);
        }

        std::vector<uint32_t> workerQueueFamilyIndices;
        workerQueueFamilyIndices.reserve(transferWorkerQueues.size());
        for (const auto *queue : transferWorkerQueues)
        {
            if (queue != nullptr)
                workerQueueFamilyIndices.push_back(queue->getParentQueueFamilyIndex());
            else
                workerQueueFamilyIndices.push_back(0);
        }
        m_workerQueueFamilyIndices = std::move(workerQueueFamilyIndices);

        for (size_t i{0}; i < transferWorkerQueues.size(); i++)
        {
            const auto *queue = transferWorkerQueues[i];

            if (queue == nullptr)
            {
                if (i > 0)
                {
                    std::ostringstream skipOss;
                    skipOss << "Skipping standard transfer worker " << i << ": no dedicated transfer queue available.";
                    core::logging::log(boost::log::trivial::warning, skipOss.str());
                }
                continue;
            }

            if (!m_workerPool->allocateWorker())
            {
                std::ostringstream poolOss;
                poolOss << "Worker pool exhausted after creating " << i
                        << " transfer worker(s). Remaining standard workers will not be created.";
                core::logging::log(boost::log::trivial::warning, poolOss.str());
                break;
            }

            // The first registered transfer worker (worker 0) is reserved for high-priority tasks.
            // Standard tasks are distributed across workers 1..N by ManagerRenderResource.
            if (i == 0)
            {
                job::worker::Worker transferWorker{job::worker::DefaultWorker{
                    THighPriorityWorkerPolicy{true, *m_device, *queue, allTransferQueueFamilyIndicesInUse},
                    "TransferWorker"}};
                m_taskManager->registerWorker(std::move(transferWorker), job::tasks::transfer::TransferTaskName);
            }
            else
            {
                job::worker::Worker transferWorker{job::worker::DefaultWorker{
                    TStandardPriorityWorkerPolicy{true, *m_device, *queue, allTransferQueueFamilyIndicesInUse},
                    "TransferWorker"}};
                m_taskManager->registerWorker(std::move(transferWorker), job::tasks::transfer::TransferTaskName);
            }
        }

        if (m_taskManager->getNumOfWorkersForType(
                Handle{.type = common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
                           job::tasks::transfer::TransferTaskName),
                       .id = 0}) == 0)
            STAR_THROW("Failed to register any transfer worker");

        const size_t total = m_taskManager->getNumOfWorkersForType(transferWorkerHandle(0));
        m_numStandardTransferWorkers = total > 1 ? total - 1 : 1;
        m_nextStandardWorker = 0;
    }

    template <size_t THighSize, size_t TStandardSize> void dispatchCreateWorkers()
    {
        createAndRegisterTransferWorkers<
            job::worker::default_worker::BusyWaitTransferTaskHandlingPolicy<THighSize>,
            job::worker::default_worker::BusyWaitTransferTaskHandlingPolicy<TStandardSize>>();
    }

    void dispatchCreateWorkersFromConfig();

    void cleanupListeners(star::core::CommandBus &cmdBus);

    void initListeners(star::core::CommandBus &cmdBus);

    Handle transferWorkerHandle(uint16_t workerIndex) const noexcept;
};
} // namespace star::service