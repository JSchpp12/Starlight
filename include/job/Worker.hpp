#pragma once

#include "tasks/Task.hpp"

#include <boost/lockfree/spsc_queue.hpp>
#include <boost/thread.hpp>

#include <iostream>
#include <memory>
#include <type_traits>
#include <vulkan/vulkan.hpp>

namespace star::job
{
class Worker
{
  public:
    enum class WorkerMode{
        Immediate, 
        FrameControlled
    };
    
    Worker() = default;
    ~Worker() = default;

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

    void queueTask(tasks::Task<>&& task)
    {
        while (!this->taskQueue.push(std::move(task)))
        {
            boost::this_thread::yield();
        }
    };

  protected:
    boost::lockfree::spsc_queue<tasks::Task<>, boost::lockfree::capacity<128>> taskQueue;
    boost::atomic<bool> shouldRun = boost::atomic<bool>(true);
    boost::thread thread = boost::thread();

    virtual void threadFunction()
    {
        std::cout << "Beginning work" << std::endl;

        while (this->shouldRun.load())
        {
            tasks::Task<> task;
            if (this->taskQueue.pop(task))
            {
                task.run();
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