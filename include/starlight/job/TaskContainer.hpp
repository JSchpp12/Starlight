#pragma once

#include "job/tasks/Task.hpp"
#include "logging/LoggingFactory.hpp"

#include <boost/atomic/atomic.hpp>
#include <boost/lockfree/queue.hpp>

#include <atomic>
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

    void queueTask(TTask &&newTask)
    {
        static_assert(std::is_nothrow_move_assignable_v<TTask> && std::is_nothrow_move_constructible_v<TTask>,
                      "TTask must be noexcept moveable for lock-free placement.");

        const uint32_t newSpace = getNextAvailableSpace();
        m_tasks[newSpace] = std::move(newTask);

        m_pending.fetch_add(1, std::memory_order_release);

        bool printedMessage = false;
        int pushTryCounter = 0;
        while (!m_queuedTasks.push(newSpace))
        {
            if (!printedMessage)
            {
                star::core::logging::warning("Failed to push queued task. Entering try loop");
                printedMessage = true;
            }

            pushTryCounter++;
            if (pushTryCounter % 50 == 0)
            {
                std::this_thread::yield();
            }
        }
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

        int pushTryCounter = 0;
        bool printedMessage = false;
        while (!m_availableSpaces.push(queuedIndex))
        {
            if (!printedMessage)
            {
                star::core::logging::warning("Failed to push available space. Entering try loop");
                if (pushTryCounter % 50 == 0)
                {
                    std::this_thread::yield();
                }
                printedMessage = true;
            }
        }

        m_pending.fetch_sub(1, std::memory_order_acq_rel);
        return tmp;
    }

    bool empty() noexcept
    {
        return m_pending.load(std::memory_order_acquire) == 0;
    }

  private:
    std::vector<TTask> m_tasks;
    boost::lockfree::queue<uint32_t, boost::lockfree::capacity<TMaxSize>> m_availableSpaces =
        boost::lockfree::queue<uint32_t, boost::lockfree::capacity<TMaxSize>>();
    boost::lockfree::queue<uint32_t, boost::lockfree::capacity<TMaxSize>> m_queuedTasks =
        boost::lockfree::queue<uint32_t, boost::lockfree::capacity<TMaxSize>>();
    std::atomic<uint32_t> m_pending{0};

    uint32_t getNextAvailableSpace()
    {
        uint32_t nextSpace = 0;

        int sleepCounter = 0;
        bool hasPrintedWarning = false;
        while (!m_availableSpaces.pop(nextSpace))
        {
            if (!hasPrintedWarning)
            {
                core::logging::log(boost::log::trivial::warning,
                                   "Task container is out of space. Consider increasing size.");
                hasPrintedWarning = true;
            }

            if (sleepCounter % 50 == 0){
                std::this_thread::yield();
            }

            sleepCounter++;
        }

        if (hasPrintedWarning)
        {
            std::ostringstream oss;
            oss << "Waiting done. Sleep looper counter: " << sleepCounter;
            core::logging::log(boost::log::trivial::info, oss.str());
        }

        return nextSpace;
    }

    void initAvailableSpaces(const uint32_t &size)
    {
        for (uint32_t i = 0; i < size; i++)
        {
            int tryCounter = 0;
            while (!m_availableSpaces.push(i))
            {
                tryCounter++;
                if (tryCounter == 25)
                {
                    STAR_THROW("Failed to initialize space");
                }
            }
        }
    }
};
} // namespace star::job