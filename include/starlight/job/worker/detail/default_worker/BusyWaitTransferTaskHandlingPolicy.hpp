#pragma once

#include "StarCommandBuffer.hpp"
#include "StarCommandPool.hpp"
#include "job/TaskContainer.hpp"
#include "job/TransferWorker.hpp"
#include "job/complete_tasks/CompleteTask.hpp"
#include "job/tasks/TransferTask.hpp"
#include "logging/LoggingFactory.hpp"

#include <boost/atomic/atomic.hpp>
#include <boost/thread.hpp>

#include <queue>
#include <sstream>

namespace star::job::worker::default_worker
{
template <size_t TQueueSize> class BusyWaitTransferTaskHandlingPolicy
{
  public:
    using TransferTask = job::tasks::transfer::TransferTask;
    using TransferPayload = job::tasks::transfer::TransferPayload;

    BusyWaitTransferTaskHandlingPolicy(bool waitForWorkToFinishBeforeExiting, core::device::StarDevice &device,
                                       StarQueue queue, const std::vector<uint32_t> &allTransferQueueFamilyIndicesInUse)
        : m_waitForWorkToFinishBeforeExiting(waitForWorkToFinishBeforeExiting), m_device(device), m_queue(queue),
          m_allTransferQueueFamilyIndicesInUse(allTransferQueueFamilyIndicesInUse),
          m_shouldRun(std::make_shared<boost::atomic<bool>>())
    {
    }

    BusyWaitTransferTaskHandlingPolicy(const BusyWaitTransferTaskHandlingPolicy &&) = delete;
    BusyWaitTransferTaskHandlingPolicy &operator=(const BusyWaitTransferTaskHandlingPolicy &&) = delete;
    BusyWaitTransferTaskHandlingPolicy(BusyWaitTransferTaskHandlingPolicy &&other) = default;
    BusyWaitTransferTaskHandlingPolicy &operator=(BusyWaitTransferTaskHandlingPolicy &&other) = default;

    ~BusyWaitTransferTaskHandlingPolicy()
    {
        if (thread.joinable())
        {
            stopThread();
        }
    }

    void init(std::string workerName, TaskContainer<complete_tasks::CompleteTask, 128> *completeMessages)
    {
        m_workerName = std::move(workerName);
        m_completeMessages = completeMessages;

        startThread();
    }

    /// Non-blocking attempt to queue a task. Returns false if the targeted container is full.
    bool queueTask(void *task)
    {
        if (!thread.joinable())
        {
            STAR_THROW("Attempted to queue task for transfer worker which has stopped or is not running");
        }

        TransferTask *typedTask = static_cast<TransferTask *>(task);
        TransferPayload &payload = *static_cast<TransferPayload *>(typedTask->getPayload());

        if (payload.priority == job::tasks::transfer::TransferPriority::High)
        {
            return m_highPriorityTasks->queueTask(std::move(*typedTask));
        }
        else
        {
            return m_standardTasks->queueTask(std::move(*typedTask));
        }
    }

    /// Blocking variant. Busy-waits until the targeted container has a free slot.
    void queueTaskBlocking(void *task)
    {
        if (!thread.joinable())
        {
            STAR_THROW("Attempted to queue task for transfer worker which has stopped or is not running");
        }

        TransferTask *typedTask = static_cast<TransferTask *>(task);
        TransferPayload &payload = *static_cast<TransferPayload *>(typedTask->getPayload());

        if (payload.priority == job::tasks::transfer::TransferPriority::High)
        {
            m_highPriorityTasks->queueTaskBlocking(std::move(*typedTask));
        }
        else
        {
            m_standardTasks->queueTaskBlocking(std::move(*typedTask));
        }
    }

    void cleanup()
    {
        stopThread();
    }

  protected:
    bool m_waitForWorkToFinishBeforeExiting = false;
    core::device::StarDevice &m_device;
    StarQueue m_queue;
    std::vector<uint32_t> m_allTransferQueueFamilyIndicesInUse;

    std::shared_ptr<boost::atomic<bool>> m_shouldRun = nullptr;
    std::shared_ptr<job::TaskContainer<TransferTask, TQueueSize>> m_highPriorityTasks = nullptr;
    std::shared_ptr<job::TaskContainer<TransferTask, TQueueSize>> m_standardTasks = nullptr;
    TaskContainer<complete_tasks::CompleteTask, 128> *m_completeMessages = nullptr;
    boost::thread thread;
    std::string m_workerName;

    std::shared_ptr<StarCommandPool> m_commandPool = nullptr;
    std::queue<std::unique_ptr<job::TransferManagerThread::ProcessRequestInfo>> m_processRequestInfos;

    void startThread()
    {
        m_shouldRun->store(true);
        m_highPriorityTasks = std::make_shared<job::TaskContainer<TransferTask, TQueueSize>>();
        m_standardTasks = std::make_shared<job::TaskContainer<TransferTask, TQueueSize>>();
        thread = boost::thread(
            [this, highPriority = m_highPriorityTasks, standard = m_standardTasks]() { threadFunction(); });
    }

    void stopThread()
    {
        m_shouldRun->store(false);
        if (thread.joinable())
            thread.join();
    }

    void wait()
    {
        boost::this_thread::yield();
    }

    void threadFunction()
    {
        logStart(m_workerName);

        auto device = m_device.getVulkanDevice();
        auto allocator = m_device.getAllocator().get();

        m_commandPool = std::make_shared<StarCommandPool>(device, m_queue.getParentQueueFamilyIndex(), true);

        for (int i = 0; i < 10; i++)
        {
            m_processRequestInfos.push(std::make_unique<job::TransferManagerThread::ProcessRequestInfo>(
                m_commandPool, std::make_unique<StarCommandBuffer>(device, 1, m_commandPool.get(),
                                                                   star::Queue_Type::Ttransfer, true, false)));
        }

        while (true)
        {
            std::optional<TransferTask> task = m_highPriorityTasks->getQueuedTask();

            if (!task.has_value())
                task = m_standardTasks->getQueuedTask();

            if (task.has_value())
            {
                TransferPayload &payload = *static_cast<TransferPayload *>(task.value().getPayload());

                assert(payload.request && "Transfer task payload must contain a request envelope");
                processRequest(*payload.request, device, allocator);

                auto message = task.value().getCompleteMessage();
                if (message.has_value())
                {
                    m_completeMessages->queueTask(std::move(message.value()));
                }
            }
            else
            {
                wait();
            }

            if (!m_shouldRun->load())
            {
                if (!m_waitForWorkToFinishBeforeExiting || (m_highPriorityTasks->empty() && m_standardTasks->empty()))
                {
                    break;
                }
            }
        }

        while (!m_processRequestInfos.empty())
        {
            std::unique_ptr<job::TransferManagerThread::ProcessRequestInfo> workingInfo =
                std::move(m_processRequestInfos.front());
            m_processRequestInfos.pop();

            if (!workingInfo->isMarkedAsAvailable())
            {
                workingInfo->commandBuffer->wait(0);
                workingInfo->markAsAvailable(device);
            }
        }

        CheckForCleanups(device, m_processRequestInfos);

        m_commandPool->cleanupRender(device);

        logStop(m_workerName);
    }

    void processRequest(job::TransferManagerThread::InterThreadRequest &request, vk::Device device,
                        VmaAllocator allocator)
    {
        std::unique_ptr<job::TransferManagerThread::ProcessRequestInfo> workingInfo =
            std::move(m_processRequestInfos.front());
        m_processRequestInfos.pop();

        EnsureInfoReady(device, *workingInfo);

        if (request.bufferTransferRequest)
        {
            assert(request.resultingBuffer.has_value() && request.resultingBuffer.value() != nullptr &&
                   "Buffer request must contain both a request and a resulting address");

            request.bufferTransferRequest->prep();

            job::TransferManagerThread::CreateBuffer(
                device, allocator, m_queue, request.gpuWorkDoneSemaphore, m_device.getPhysicalDevice().getProperties(),
                m_allTransferQueueFamilyIndicesInUse, *workingInfo, request.bufferTransferRequest.get(),
                request.resultingBuffer.value(), request.gpuDoneNotificationToMain, request.workSyncInfo);
        }
        else if (request.textureTransferRequest)
        {
            assert(request.resultingTexture.has_value() && request.resultingTexture.value() != nullptr &&
                   "Texture request must contain both a request and a resulting address");

            request.textureTransferRequest->prep();

            core::logging::log(boost::log::trivial::info, "Creating Texture");

            job::TransferManagerThread::CreateTexture(
                device, allocator, m_queue, request.gpuWorkDoneSemaphore, m_device.getPhysicalDevice().getProperties(),
                m_allTransferQueueFamilyIndicesInUse, *workingInfo, request.textureTransferRequest.get(),
                request.resultingTexture.value(), request.gpuDoneNotificationToMain);
        }

        m_processRequestInfos.push(std::move(workingInfo));

        request.gpuDoneNotificationToMain->store(true);
        request.gpuDoneNotificationToMain->notify_all();
    }

    static void EnsureInfoReady(vk::Device device, job::TransferManagerThread::ProcessRequestInfo &info)
    {
        if (!info.isMarkedAsAvailable())
        {
            info.commandBuffer->begin(0);
            info.markAsAvailable(device);
        }
        else
        {
            info.commandBuffer->begin(0);
        }
    }

    static void CheckForCleanups(
        vk::Device device, std::queue<std::unique_ptr<job::TransferManagerThread::ProcessRequestInfo>> &processingInfos)
    {
        std::queue<std::unique_ptr<job::TransferManagerThread::ProcessRequestInfo>> readyInfos;
        std::queue<std::unique_ptr<job::TransferManagerThread::ProcessRequestInfo>> notReadyInfos;

        while (!processingInfos.empty())
        {
            std::unique_ptr<job::TransferManagerThread::ProcessRequestInfo> workingInfo =
                std::move(processingInfos.front());
            processingInfos.pop();

            if (!workingInfo->isMarkedAsAvailable() && workingInfo->commandBuffer->isFenceReady(0))
            {
                workingInfo->markAsAvailable(device);
                readyInfos.push(std::move(workingInfo));
            }
            else
            {
                notReadyInfos.push(std::move(workingInfo));
            }
        }

        while (!readyInfos.empty())
        {
            processingInfos.push(std::move(readyInfos.front()));
            readyInfos.pop();
        }

        while (!notReadyInfos.empty())
        {
            processingInfos.push(std::move(notReadyInfos.front()));
            notReadyInfos.pop();
        }
    }

    static void writeLog(std::string_view workerName, boost::log::trivial::severity_level level, std::string message)
    {
        std::ostringstream oss;
        oss << workerName << " - " << message;

        core::logging::log(std::move(level), oss.str());
    }

    static void logStart(std::string_view workerName)
    {
        writeLog(workerName, boost::log::trivial::info, "Beginning Work");
    }

    static void logStop(std::string_view workerName)
    {
        writeLog(workerName, boost::log::trivial::info, "Exiting");
    }
};
} // namespace star::job::worker::default_worker
