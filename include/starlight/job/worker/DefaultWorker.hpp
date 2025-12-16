#pragma once

#include "TaskContainer.hpp"

#include "core/logging/LoggingFactory.hpp"
#include "default_worker/detail/ThreadTaskHandlingPolicies.hpp"

#include <vulkan/vulkan.hpp>

#include <concepts>
#include <iostream>
#include <memory>
#include <type_traits>

namespace star::job::worker
{

template <typename TThreadPolicy>
concept ThreadFunctionPolicyLike =
    requires(std::string workerName, TThreadPolicy threadPolicy,
             TaskContainer<complete_tasks::CompleteTask, 128> *completeMessages, void *rawTask) {
        { threadPolicy.init(workerName, completeMessages) } -> std::same_as<void>;
        { threadPolicy.queueTask(rawTask) } -> std::same_as<void>;
        { threadPolicy.cleanup() } -> std::same_as<void>;
    };

template <ThreadFunctionPolicyLike TThreadPolicy> class DefaultWorker
{
  public:
    DefaultWorker(TThreadPolicy threadPolicy, std::string workerName)
        : m_threadPolicy(std::move(threadPolicy)), m_workerName(std::move(workerName))
    {
    }
    DefaultWorker(const DefaultWorker &) = delete;
    DefaultWorker &operator=(const DefaultWorker &) = delete;
    DefaultWorker(DefaultWorker &&other) = default;
    DefaultWorker &operator=(DefaultWorker &&) = default;

    void cleanup()
    {
        m_threadPolicy.cleanup();
    }

    void queueTask(void *task)
    {
        m_threadPolicy.queueTask(task);
    }

    void setCompleteMessageCommunicationStructure(TaskContainer<complete_tasks::CompleteTask, 128> *completeMessages)
    {
        m_threadPolicy.init(m_workerName, completeMessages);
    }

  protected:
    TThreadPolicy m_threadPolicy;
    std::string m_workerName;
};
} // namespace star::job::worker