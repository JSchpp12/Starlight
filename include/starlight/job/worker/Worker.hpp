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
        virtual bool doIsTaskQueueFull(const void *task) const noexcept = 0;
        virtual void doCleanup() = 0;
        virtual void doQueueTask(void *task) = 0;
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

    void queueTask(void *task)
    {
        m_pimpl->doQueueTask(task);
    }

    void setCompleteMessageCommunicationStructure(TaskContainer<complete_tasks::CompleteTask, 128> *completeMessages)
    {
        m_pimpl->doSetCompleteMessageCommunicationStructure(completeMessages);
    }

    bool isTaskQueueFull(const void *task) const noexcept
    {
        return m_pimpl->doIsTaskQueueFull(task);
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
        void doQueueTask(void *task) override
        {
            m_worker.queueTask(task);
        }
        bool doIsTaskQueueFull(const void *task) const noexcept override
        {
            return m_worker.isTaskQueueFull(task);
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