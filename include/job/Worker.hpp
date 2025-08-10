#pragma once

#include "Task.hpp"
#include "IWorkerBase.hpp"

#include <boost/lockfree/spsc_queue.hpp>
#include <boost/thread.hpp>

#include <iostream>
#include <memory>
#include <type_traits>
#include <vulkan/vulkan.hpp>

namespace star::Job
{
template <typename T, typename TTask> class Worker : public IWorkerBase
{
  public:
    Worker() = default;
    virtual ~Worker() = default;

    void stop() override
    {
        this->shouldRun.store(false);
        if (this->thread.joinable())
            this->thread.join();
    }

    void start() override
    {
        this->shouldRun.store(true);
        this->thread = boost::thread(&Worker::threadFunction, this);
    }

    Worker(const Worker &) = delete;
    Worker &operator=(const Worker &) = delete;
    Worker(Worker &&) = default;
    Worker &operator=(Worker &&) = default;

    void queueTask(std::shared_ptr<TTask> task)
    {
        while (!this->taskQueue.push(task))
        {
            boost::this_thread::yield();
        }
    };

  protected:
    boost::lockfree::spsc_queue<std::shared_ptr<TTask>, boost::lockfree::capacity<128>> taskQueue;
    boost::atomic<bool> shouldRun = boost::atomic<bool>(true);
    boost::thread thread = boost::thread();

    virtual void threadFunction()
    {
        std::cout << "Beginning work" << std::endl;

        while (this->shouldRun.load())
        {
            std::shared_ptr<Task<T>> task = nullptr;
            if (this->taskQueue.pop(task))
            {
                task->execute();
            }
            else
            {
                boost::this_thread::sleep_for(boost::chrono::milliseconds(2));
            }
        }

        std::cout << "Exiting" << std::endl;
    }
};
} // namespace star::Job