#pragma once

#include "TaskContainer.hpp"
#include "TransferWorker.hpp"
#include "complete_tasks/CompleteTask.hpp"
#include <boost/lockfree/stack.hpp>
#include <boost/thread.hpp>

#include <iostream>
#include <memory>
#include <type_traits>
#include <vulkan/vulkan.hpp>

namespace star::job::worker
{
class Worker
{
  public:
    Worker() = default;
    Worker(boost::lockfree::stack<complete_tasks::CompleteTask<>, boost::lockfree::capacity<128>> *completeMessages)
        : m_completeMessages(completeMessages)
    {
    }
    virtual ~Worker() = default;

    void stop()
    {
        this->shouldRun->store(false);
        if (this->thread.joinable())
            this->thread.join();
    }

    void start()
    {
        this->shouldRun->store(true);
        this->thread = boost::thread(&Worker::threadFunction, this);
    }

    Worker(const Worker &) = delete;
    Worker &operator=(const Worker &) = delete;
    Worker(Worker &&other)
        : m_tasks(std::move(other.m_tasks)), shouldRun(std::move(other.shouldRun)),
          m_completeMessages(std::move(other.m_completeMessages)), thread(std::move(other.thread)) {};
    Worker &operator=(Worker &&) = default;

    void queueTask(tasks::Task<> task)
    {
        m_tasks->queueTask(std::move(task));
    }

    void setCompleteMessageCommunicationStructure(
        boost::lockfree::stack<complete_tasks::CompleteTask<>, boost::lockfree::capacity<128>> *completeMessages)
    {
        m_completeMessages = completeMessages;
    }

  protected:
    std::unique_ptr<job::TaskContainer<tasks::Task<>, 128>> m_tasks =
        std::unique_ptr<job::TaskContainer<tasks::Task<>, 128>>(new job::TaskContainer<tasks::Task<>, 128>());
    std::unique_ptr<boost::atomic<bool>> shouldRun =
        std::unique_ptr<boost::atomic<bool>>(new boost::atomic<bool>(true));
    boost::lockfree::stack<complete_tasks::CompleteTask<>, boost::lockfree::capacity<128>> *m_completeMessages =
        nullptr;
    boost::thread thread = boost::thread();

    virtual void threadFunction();
};
} // namespace star::job::worker