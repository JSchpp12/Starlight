#pragma once

#include "complete_tasks/CompleteTask.hpp"

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
        virtual void doStart() = 0;
        virtual void doStop() = 0;
        virtual void doQueueTask(void *task) = 0;
        virtual void doSetCompleteMessageCommunicationStructure(
            boost::lockfree::stack<complete_tasks::CompleteTask, boost::lockfree::capacity<128>> *completeMessages) = 0;
    };

    template <typename TWorker>
    Worker(TWorker worker) : m_impl(std::make_unique<WorkerModel<TWorker>>(std::move(worker)))
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
        return m_impl.get();
    }
    void stop()
    {
        m_impl->doStop();
    }

    void start()
    {
        m_impl->doStart();
    }

    void queueTask(void *task)
    {
        m_impl->doQueueTask(task);
    }

    void setCompleteMessageCommunicationStructure(
        boost::lockfree::stack<complete_tasks::CompleteTask, boost::lockfree::capacity<128>> *completeMessages)
    {
        m_impl->doSetCompleteMessageCommunicationStructure(completeMessages);
    }

  private:
    template <typename TWorker> struct WorkerModel : public WorkerConcept
    {
        TWorker m_worker;
        explicit WorkerModel(TWorker worker) : m_worker(std::move(worker))
        {
        }
        WorkerModel(const WorkerModel &) = delete;
        WorkerModel &operator=(const WorkerModel &) = delete;
        WorkerModel(WorkerModel &&) = default;
        WorkerModel &operator=(WorkerModel &&) = default;
        virtual ~WorkerModel() = default;

        void doStart() override
        {
            m_worker.start();
        }

        void doStop() override
        {
            m_worker.stop();
        }

        void doQueueTask(void *task) override
        {
            m_worker.queueTask(task);
        }

        void doSetCompleteMessageCommunicationStructure(
            boost::lockfree::stack<complete_tasks::CompleteTask, boost::lockfree::capacity<128>> *completeMessages)
            override
        {
            m_worker.setCompleteMessageCommunicationStructure(completeMessages);
        }
    };

    std::unique_ptr<WorkerConcept> m_impl;
};
} // namespace star::job::worker