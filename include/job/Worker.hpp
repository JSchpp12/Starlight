#pragma once

#include "complete_tasks/CompleteTask.hpp"
#include "tasks/Task.hpp"
#include "TransferWorker.hpp"


#include <boost/lockfree/stack.hpp>
#include <boost/thread.hpp>

#include <iostream>
#include <memory>
#include <type_traits>
#include <vulkan/vulkan.hpp>

namespace star::job::workers
{
class Worker
{
  public:
    // enum class WorkerMode
    // {
    //     Immediate,
    //     FrameControlled
    // };

    Worker() = default;
    Worker(boost::lockfree::stack<complete_tasks::CompleteTask<>, boost::lockfree::capacity<128>> *completeMessages)
        : m_completeMessages(completeMessages)
    {
    }
    virtual ~Worker() = default;

    void stop()
    {
        this->shouldRun.store(false);
        if (this->thread.joinable())
            this->thread.join();
    }

    void start()
    {
        this->shouldRun.store(true);
        this->thread = boost::thread(&Worker::threadFunction, this);
    }

    Worker(const Worker &) = delete;
    Worker &operator=(const Worker &) = delete;
    Worker(Worker &&) = default;
    Worker &operator=(Worker &&) = default;

    void queueTask(tasks::Task<> &&task)
    {
        while (!this->taskQueue.push(std::move(task)))
        {
            boost::this_thread::yield();
        }
    };

    void setCompleteMessageCommunicationStructure(boost::lockfree::stack<complete_tasks::CompleteTask<>, boost::lockfree::capacity<128>> *completeMessages){
        m_completeMessages = completeMessages; 
    }

  protected:
    boost::lockfree::spsc_queue<tasks::Task<>, boost::lockfree::capacity<128>> taskQueue;
    boost::lockfree::stack<complete_tasks::CompleteTask<>, boost::lockfree::capacity<128>> *m_completeMessages = nullptr;
    boost::atomic<bool> shouldRun = boost::atomic<bool>(true);
    boost::thread thread = boost::thread();

    virtual void threadFunction();
};
} // namespace star::job