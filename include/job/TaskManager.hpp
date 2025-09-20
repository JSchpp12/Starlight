#pragma once

#include "FrameScheduler.hpp"
#include "job/worker/Worker.hpp"
#include "complete_tasks/CompleteTask.hpp"

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
                  boost::lockfree::stack<job::complete_tasks::CompleteTask<>, boost::lockfree::capacity<128>>>()),
          m_defaultWorker(CreateDefaultWorker(m_completeTasks.get())) {};
    ~TaskManager() = default;

    TaskManager(const TaskManager &) = delete;
    TaskManager &operator=(const TaskManager &) = delete;

    TaskManager(TaskManager &&) = default;
    TaskManager &operator=(TaskManager &&) = default;

    worker::WorkerBase &registerWorker(const std::type_index &taskType)
    {
        auto worker = std::make_shared<worker::Worker<>>(m_completeTasks.get());
        worker::WorkerBase *raw = worker.get();
        m_workers[taskType].emplace_back(worker);

        m_frameSchedulers[taskType] = FrameScheduler();

        return *raw;
    }

    worker::WorkerBase &registerWorker(const std::type_index &taskType, std::shared_ptr<worker::WorkerBase> newWorker)
    {
        newWorker->setCompleteMessageCommunicationStructure(m_completeTasks.get());

        auto *raw = newWorker.get();
        m_workers[taskType].push_back(newWorker);
        return *raw;
    }

    void startAll()
    {
        for (auto &[type, list] : m_workers)
        {
            for (auto &w : list)
            {
                w->start();
            }
        }

        m_defaultWorker->start();
    }

    void stopAll()
    {
        for (auto &[type, list] : m_workers)
        {
            for (auto &w : list)
            {
                w->stop();
            }
        }

        m_defaultWorker->stop();
    }

    void submitTask(tasks::Task<> &&newTask)
    {
        worker::WorkerBase *worker = getWorker(newTask.getType());
        if (worker == nullptr)
        {
            worker = m_defaultWorker.get();
        }

        worker->queueTask(std::move(newTask));
    }

    void scheduleTaskForFrame(uint64_t &targetFrameIndex, std::type_index &taskType, tasks::Task<> &&newTask)
    {
        m_frameSchedulers[taskType].scheduleTasksForFrame(targetFrameIndex, std::move(newTask));
    }

    void dispatchTasksForFrame(std::type_index &taskType, const uint64_t &frameIndex)
    {
        auto scheduler = m_frameSchedulers.find(taskType);

        if (scheduler != m_frameSchedulers.end())
        {
            worker::WorkerBase *worker = getWorker(taskType);
            if (worker != nullptr)
            {
                for (auto &task : scheduler->second.fetchTasksForFrame(frameIndex))
                {
                    worker->queueTask(std::move(task));
                }
            }
        }
    }

    void dispatchTasksForFrame(const uint64_t &targetFrameIndex)
    {
        for (auto &[type, scheduler] : m_frameSchedulers)
        {
            for (auto &task : scheduler.fetchTasksForFrame(targetFrameIndex))
            {
                worker::WorkerBase *worker = getWorker(type);
                assert(worker != nullptr && "Worker for this type was not found.");

                worker->queueTask(std::move(task));
            }
        }
    }

    boost::lockfree::stack<job::complete_tasks::CompleteTask<>, boost::lockfree::capacity<128>> *getCompleteMessages()
    {
        return m_completeTasks.get();
    }

  private:
    std::unordered_map<std::type_index, std::vector<std::shared_ptr<worker::WorkerBase>>> m_workers =
        std::unordered_map<std::type_index, std::vector<std::shared_ptr<worker::WorkerBase>>>();

    std::unique_ptr<boost::lockfree::stack<job::complete_tasks::CompleteTask<>, boost::lockfree::capacity<128>>>
        m_completeTasks = nullptr;

    std::unordered_map<std::type_index, FrameScheduler> m_frameSchedulers;

    std::unique_ptr<worker::WorkerBase> m_defaultWorker = nullptr;

    worker::WorkerBase *getWorker(const std::type_index &taskType, const size_t &index = 0)
    {
        auto it = m_workers.find(taskType);
        if (it == m_workers.end() || index >= it->second.size())
        {
            return nullptr;
        }

        return it->second[index].get();
    }

    static std::unique_ptr<worker::WorkerBase> CreateDefaultWorker(
        boost::lockfree::stack<job::complete_tasks::CompleteTask<>, boost::lockfree::capacity<128>> *completeMessages)
    {
        return std::make_unique<worker::Worker<>>(completeMessages);
    }
};
} // namespace star::job