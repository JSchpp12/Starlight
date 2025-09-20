#pragma once

#include "TaskContainer.hpp"
#include "WorkerBase.hpp"
#include "complete_tasks/CompleteTask.hpp"
#include <boost/lockfree/stack.hpp>
#include <boost/thread.hpp>

#include <iostream>
#include <memory>
#include <type_traits>
#include <vulkan/vulkan.hpp>

namespace star::job::worker
{
template <size_t TQueueSize = 128> class Worker : public WorkerBase
{
  public:
    Worker() = default;
    Worker(
        boost::lockfree::stack<complete_tasks::CompleteTask<>, boost::lockfree::capacity<TQueueSize>> *completeMessages)
        : m_completeMessages(completeMessages)
    {
    }
    virtual ~Worker() = default;
    Worker(const Worker &other) : m_tasks(other.m_tasks), m_completeMessages(other.m_completeMessages)
    {
        // create shared resources
        if (other.shouldRun->load())
        {
            start();
        }
    }
    Worker &operator=(const Worker &other)
    {
        if (this != &other)
        {
            m_tasks = other.m_tasks;
            m_completeMessages = other.m_completeMessages;

            if (other.shouldRun->load())
            {
                start();
            }
        }

        return *this;
    };
    Worker(Worker &&other)
        : m_tasks(std::move(other.m_tasks)), shouldRun(std::move(other.shouldRun)),
          m_completeMessages(std::move(other.m_completeMessages)), thread(std::move(other.thread))
    {
    }
    Worker &operator=(Worker &&other)
    {
        if (this != &other)
        {
            m_tasks = std::move(other.m_tasks);
            shouldRun = std::move(other.shouldRun);
            m_completeMessages = std::move(other.m_completeMessages);
            thread = std::move(other.thread);
        }

        return *this;
    }

    void stop() override
    {
        this->shouldRun->store(false);
        if (this->thread.joinable())
            this->thread.join();
    }

    void start() override
    {
        this->shouldRun->store(true);
        this->thread = boost::thread(&Worker::threadFunction, this);
    }

    void queueTask(tasks::Task<> task) override
    {
        m_tasks->queueTask(std::move(task));
    }

    void setCompleteMessageCommunicationStructure(
        boost::lockfree::stack<complete_tasks::CompleteTask<>, boost::lockfree::capacity<128>> *completeMessages) override
    {
        m_completeMessages = completeMessages;
    }

  protected:
    std::shared_ptr<job::TaskContainer<tasks::Task<>, TQueueSize>> m_tasks =
        std::shared_ptr<job::TaskContainer<tasks::Task<>, TQueueSize>>(
            new job::TaskContainer<tasks::Task<>, TQueueSize>());
    std::unique_ptr<boost::atomic<bool>> shouldRun =
        std::unique_ptr<boost::atomic<bool>>(new boost::atomic<bool>(true));
    boost::lockfree::stack<complete_tasks::CompleteTask<>, boost::lockfree::capacity<128>> *m_completeMessages =
        nullptr;
    boost::thread thread = boost::thread();

    virtual void threadFunction()
    {
        assert(m_completeMessages != nullptr);

        std::cout << "Beginning work" << std::endl;

        while (this->shouldRun->load())
        {
            std::optional<tasks::Task<>> task = m_tasks->getQueuedTask();

            if (task.has_value())
            {
                task.value().run();

                auto message = task.value().getCompleteMessage();
                if (message.has_value())
                {
                    m_completeMessages->push(std::move(message.value()));
                }
            }
            else
            {
                boost::this_thread::sleep_for(boost::chrono::milliseconds(2));
            }
        }

        std::cout << "Exiting" << std::endl;
    }
};
} // namespace star::job::worker