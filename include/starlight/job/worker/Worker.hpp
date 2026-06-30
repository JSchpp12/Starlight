#pragma once

#include "complete_tasks/CompleteTask.hpp"
#include "job/TaskContainer.hpp"

#include <boost/lockfree/stack.hpp>

#include <memory>

namespace star::job::worker
{
class Worker
{
  public:
    struct WorkerConcept
    {
        virtual ~WorkerConcept() = default;
        virtual void doCleanup() = 0;
        virtual bool doQueueTask(void *task) = 0;
        virtual void doQueueTaskBlocking(void *task) = 0;
        virtual void doSetCompleteMessageCommunicationStructure(
            TaskContainer<complete_tasks::CompleteTask, 128> *completeMessages) = 0;
    };

    template <typename TWorker>
    Worker(TWorker &&worker) : m_pimpl(std::make_unique<WorkerModel<TWorker>>(std::forward<TWorker>(worker)))
    {
    }
    Worker() = delete;
    Worker(const Worker &) = delete;
    Worker &operator=(const Worker &) = delete;
    Worker(Worker &&) noexcept = default;
    Worker &operator=(Worker &&) noexcept = default;
    ~Worker() = default;

    WorkerConcept *getRawConcept()
    {
        return m_pimpl.get();
    }

    void cleanup()
    {
        m_pimpl->doCleanup();
    }

    /// Queue a task for the worker to process.
    ///
    /// Contract: the caller retains ownership of the pointed-to task until this call returns.
    /// Implementations MUST move the task into their own storage synchronously (e.g. via
    /// TaskContainer::queueTask(std::move(*static_cast<TTask*>(task)))) and MUST NOT retain
    /// the void* for asynchronous dereference by the worker thread — the pointer is only
    /// valid for the duration of this call.
    ///
    /// Returns false if the worker's task container is full (non-blocking); true on success.
    bool queueTask(void *task)
    {
        return m_pimpl->doQueueTask(task);
    }

    /// Blocking variant of queueTask. Busy-waits until the worker's task container has a free slot.
    void queueTaskBlocking(void *task)
    {
        m_pimpl->doQueueTaskBlocking(task);
    }

    void setCompleteMessageCommunicationStructure(TaskContainer<complete_tasks::CompleteTask, 128> *completeMessages)
    {
        m_pimpl->doSetCompleteMessageCommunicationStructure(completeMessages);
    }

  private:
    template <typename TWorker> struct WorkerModel : public WorkerConcept
    {
        TWorker m_worker;
        template <typename T> explicit WorkerModel(T &&worker) : m_worker(std::forward<T>(worker))
        {
        }
        WorkerModel(const WorkerModel &) = delete;
        WorkerModel &operator=(const WorkerModel &) = delete;
        WorkerModel(WorkerModel &&) = delete;
        WorkerModel &operator=(WorkerModel &&) = delete;
        virtual ~WorkerModel() = default;

        void doCleanup() override
        {
            m_worker.cleanup();
        }
        bool doQueueTask(void *task) override
        {
            return m_worker.queueTask(task);
        }
        void doQueueTaskBlocking(void *task) override
        {
            m_worker.queueTaskBlocking(task);
        }

        void doSetCompleteMessageCommunicationStructure(
            TaskContainer<complete_tasks::CompleteTask, 128> *completeMessages) override
        {
            m_worker.setCompleteMessageCommunicationStructure(completeMessages);
        }
    };

    std::unique_ptr<WorkerConcept> m_pimpl;
};
} // namespace star::job::worker