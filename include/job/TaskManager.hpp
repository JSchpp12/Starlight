#pragma once

#include "FrameScheduler.hpp"
#include "complete_tasks/CompleteTask.hpp"
#include "job/worker/Worker.hpp"

#include <boost/lockfree/stack.hpp>

#include <memory>
#include <typeindex>
#include <unordered_map>

namespace star::job
{
class TaskManager
{
  public:
    TaskManager()
        : m_completeTasks(
              std::make_unique<
                  boost::lockfree::stack<job::complete_tasks::CompleteTask<>, boost::lockfree::capacity<128>>>()) {};
    ~TaskManager() = default;

    TaskManager(const TaskManager &) = delete;
    TaskManager &operator=(const TaskManager &) = delete;

    TaskManager(TaskManager &&) = default;
    TaskManager &operator=(TaskManager &&) = default;

    worker::Worker &registerWorker(const std::type_index &taskType, worker::Worker newWorker)
    {
        newWorker.setCompleteMessageCommunicationStructure(m_completeTasks.get());

        if (m_allRunning)
        {
            newWorker.start();
        }

        m_workers[taskType].emplace_back(std::move(newWorker));
        return m_workers[taskType].back();
    }

    void startAll()
    {
        for (auto &[type, list] : m_workers)
        {
            for (auto &w : list)
            {
                w.start();
            }
        }

        m_allRunning = true;
    }

    void stopAll()
    {
        for (auto &[type, list] : m_workers)
        {
            for (auto &w : list)
            {
                w.stop();
            }
        }

        m_allRunning = false;
    }

    template <typename TTask> void submitTask(TTask &&newTask)
    {
        worker::Worker *worker = getWorker(typeid(TTask));
        if (worker == nullptr)
        {
            throw std::runtime_error("Failed to find corresponding worker for the provided task type");
        }

        TTask *storedTask = new TTask(std::move(newTask));
        worker->queueTask(static_cast<void *>(storedTask));
    }

    // void scheduleTaskForFrame(uint64_t &targetFrameIndex, std::type_index &taskType, tasks::Task<> &&newTask)
    // {
    //     m_frameSchedulers[taskType].scheduleTasksForFrame(targetFrameIndex, std::move(newTask));
    // }

    // void dispatchTasksForFrame(std::type_index &taskType, const uint64_t &frameIndex)
    // {
    //     auto scheduler = m_frameSchedulers.find(taskType);

    //     if (scheduler != m_frameSchedulers.end())
    //     {
    //         worker::WorkerBase *worker = getWorker(taskType);
    //         if (worker != nullptr)
    //         {
    //             for (auto &task : scheduler->second.fetchTasksForFrame(frameIndex))
    //             {
    //                 worker->queueTask(std::move(task));
    //             }
    //         }
    //     }
    // }

    // void dispatchTasksForFrame(const uint64_t &targetFrameIndex)
    // {
    //     for (auto &[type, scheduler] : m_frameSchedulers)
    //     {
    //         for (auto &task : scheduler.fetchTasksForFrame(targetFrameIndex))
    //         {
    //             worker::WorkerBase *worker = getWorker(type);
    //             assert(worker != nullptr && "Worker for this type was not found.");

    //             worker->queueTask(std::move(task));
    //         }
    //     }
    // }

    boost::lockfree::stack<job::complete_tasks::CompleteTask<>, boost::lockfree::capacity<128>> *getCompleteMessages()
    {
        return m_completeTasks.get();
    }

    bool isThereWorkerForTask(const std::type_index &taskType)
    {
        const worker::Worker *found = getWorker(taskType);
        if (found == nullptr)
        {
            return false;
        }

        return true;
    }

  private:
    std::unordered_map<std::type_index, std::vector<worker::Worker>> m_workers =
        std::unordered_map<std::type_index, std::vector<worker::Worker>>();

    std::unique_ptr<boost::lockfree::stack<job::complete_tasks::CompleteTask<>, boost::lockfree::capacity<128>>>
        m_completeTasks = nullptr;

    // std::unordered_map<std::type_index, FrameScheduler> m_frameSchedulers;
    bool m_allRunning = false;

    worker::Worker *getWorker(const std::type_index &taskType, const size_t &index = 0)
    {
        auto it = m_workers.find(taskType);
        if (it == m_workers.end() || index >= it->second.size())
        {
            return nullptr;
        }

        return &it->second[index];
    }
};
} // namespace star::job