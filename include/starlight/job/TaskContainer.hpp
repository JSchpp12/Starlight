#pragma once

#include "job/tasks/Task.hpp"
#include "logging/LoggingFactory.hpp"

#include <boost/atomic/atomic.hpp>
#include <boost/lockfree/stack.hpp>

#include <concepts>
#include <optional>
#include <sstream>
#include <thread>
#include <vector>

namespace star::job
{
template <typename TTask>
concept TTaskLike = requires(TTask t) {
    { t.reset() } -> std::same_as<void>;
};

template <TTaskLike TTask, size_t TMaxSize> class TaskContainer
{
  public:
    TaskContainer() : m_tasks(TMaxSize)
    {
        initAvailableSpaces(TMaxSize);
    }
    TaskContainer(const TaskContainer &other) = delete;
    TaskContainer &operator=(const TaskContainer &) = delete;
    TaskContainer(TaskContainer &&) = delete;
    TaskContainer &operator=(TaskContainer &&) = delete;
    ~TaskContainer() = default;

    void queueTask(TTask newTask)
    {
        const uint32_t newSpace = getNextAvailableSpace();

        m_tasks[newSpace] = std::move(newTask);
        m_queuedTasks.push(newSpace);
    }

    std::optional<TTask> getQueuedTask()
    {
        uint32_t queuedIndex = 0;
        if (!m_queuedTasks.pop(queuedIndex))
        {
            return std::nullopt;
        }

        assert(queuedIndex < TMaxSize && "Popped queued task is beyond available range");

        auto tmp = std::make_optional<TTask>(std::move(m_tasks[queuedIndex]));
        if (!m_availableSpaces.push(queuedIndex))
        {
            throw std::runtime_error("Failed to push available space");
        }
        return tmp;
    }

  private:
    std::vector<TTask> m_tasks;
    boost::lockfree::stack<uint32_t, boost::lockfree::capacity<TMaxSize>> m_availableSpaces =
        boost::lockfree::stack<uint32_t, boost::lockfree::capacity<TMaxSize>>();
    boost::lockfree::stack<uint32_t, boost::lockfree::capacity<TMaxSize>> m_queuedTasks =
        boost::lockfree::stack<uint32_t, boost::lockfree::capacity<TMaxSize>>();

    uint32_t getNextAvailableSpace()
    {
        uint32_t nextSpace = 0;
        if (!m_availableSpaces.pop(nextSpace))
        {
            core::logging::log(boost::log::trivial::warning,
                               "Task container is out of space. Consider increasing size.");

            int sleepCounter = 0;
            while (!m_availableSpaces.pop(nextSpace))
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                sleepCounter++;

                if (sleepCounter == 1000)
                {
                    throw std::runtime_error("Failed to wait for free spot to become available");
                }
            }

            {
                std::ostringstream oss;
                oss << "Waiting done. Sleep looper counter: " << sleepCounter;
                core::logging::log(boost::log::trivial::info, oss.str());
            }
        }

        return nextSpace;
    }

    void initAvailableSpaces(const uint32_t &size)
    {
        for (uint32_t i = 0; i < size; i++)
        {
            if (!m_availableSpaces.push(i))
            {
                throw std::runtime_error("Failed to initialize space");
            }
        }
    }
};
} // namespace star::job