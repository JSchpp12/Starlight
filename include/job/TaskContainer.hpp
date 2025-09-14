#pragma once

#include "job/tasks/Task.hpp"

#include <boost/lockfree/queue.hpp>

#include <queue>
#include <vector>
#include <optional>

namespace star::job
{
template <typename TTask, size_t TMaxSize> class TaskContainer
{
  public:
    TaskContainer() : m_tasks(std::vector<tasks::Task<>>(TMaxSize))
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

    void freeTask(const uint32_t &taskHandle)
    {
        assert(taskHandle <= TMaxSize && "Task handle is beyond range of container size");
        m_tasks[taskHandle].reset();
        m_availableSpaces.push(taskHandle);
    }

    std::optional<TTask> getQueuedTask()
    {
        uint32_t queuedIndex = 0;
        if (!m_queuedTasks.pop(queuedIndex))
        {
            return std::nullopt;
        }

        assert(queuedIndex <= TMaxSize && "Popped queued task is beyond available range"); 

        return std::make_optional<TTask>(std::move(m_tasks[queuedIndex])); 
    }

  private:
    std::vector<tasks::Task<>> m_tasks = std::vector<tasks::Task<>>(TMaxSize);
    boost::lockfree::queue<uint32_t, boost::lockfree::capacity<TMaxSize>> m_availableSpaces =
        boost::lockfree::queue<uint32_t, boost::lockfree::capacity<TMaxSize>>();
    boost::lockfree::queue<uint32_t, boost::lockfree::capacity<TMaxSize>> m_queuedTasks =
        boost::lockfree::queue<uint32_t, boost::lockfree::capacity<TMaxSize>>();

    uint32_t getNextAvailableSpace()
    {
        uint32_t nextSpace = 0;
        m_availableSpaces.pop(nextSpace);

        return nextSpace;
    }

    void initAvailableSpaces(const uint32_t &size)
    {
        for (uint32_t i = 0; i < size; i++)
        {
            m_availableSpaces.push(i);
        }
    }
};
} // namespace star::job