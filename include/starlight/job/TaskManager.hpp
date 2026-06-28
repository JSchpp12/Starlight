#pragma once

#include "FrameScheduler.hpp"
#include "complete_tasks/CompleteTask.hpp"
#include "job/worker/Worker.hpp"

#include <star_common/Handle.hpp>
#include <star_common/HandleTypeRegistry.hpp>

#include <absl/container/flat_hash_map.h>

#include <atomic>
#include <memory>
#include <string_view>
#include <typeindex>

namespace star::job
{
class TaskManager
{
  public:
    TaskManager() : m_completeTasks(std::make_unique<job::TaskContainer<job::complete_tasks::CompleteTask, 128>>()){};
    TaskManager(const TaskManager &) = delete;
    TaskManager &operator=(const TaskManager &) = delete;
    TaskManager(TaskManager &&) = default;
    TaskManager &operator=(TaskManager &&) = default;
    ~TaskManager() = default;
    Handle registerNewTaskType(std::string_view taskName) noexcept;

    Handle registerWorker(worker::Worker newWorker, std::string_view taskName) noexcept;

    void registerWorker(worker::Worker newWorker, Handle &registeredTaskTypeHanle) noexcept;

    void cleanup() noexcept;

    size_t getNumOfWorkersForType(const Handle &registeredTaskType) const noexcept;

    /// <summary>
    /// Submit a task to one of the workers for the provided handle type
    /// </summary>
    /// <typeparam name="TTask"></typeparam>
    /// <param name="newTask"></param>
    /// <param name="registeredTaskType">Contains the type which will be used to select worker pool. ID will be used to
    /// select specific worker from that pool</param>
    template <typename TTask> void submitTask(TTask &&newTask, const Handle &registeredTaskType)
    {
        worker::Worker *worker = getWorker(registeredTaskType);
        if (worker == nullptr)
        {
            std::ostringstream oss; 
            oss << "Failed to find corresponding worker for the task type provided: " << registeredTaskType.getType(); 
            STAR_THROW(oss.str()); 
        }

        worker->queueTask(static_cast<void *>(&newTask));
    }

    template <typename TTask> void submitTask(TTask &&newTask, std::string_view taskName)
    {
        Handle handle = {.type = common::HandleTypeRegistry::instance().getTypeGuaranteedExist(taskName), .id = 0};

        submitTask<TTask>(std::forward<TTask>(newTask), handle);
    }

    /// <summary>
    /// Submit a task to the next available worker in the pool for the given task type name, using round-robin
    /// distribution
    /// </summary>
    template <typename TTask>
    void submitTaskRoundRobin(TTask &&newTask, std::string_view taskName)
    {
        const uint16_t type = common::HandleTypeRegistry::instance().getTypeGuaranteedExist(taskName);

        auto *pool = getWorkerPool(type);
        if (pool == nullptr || pool->empty())
        {
            throw std::runtime_error("No workers registered for task type: " + std::string(taskName));
        }

        auto it = m_nextWorkerIndex.find(type);
        if (it == m_nextWorkerIndex.end())
        {
            it = m_nextWorkerIndex.emplace(type, 0).first;
        }

        const size_t index = it->second;
        it->second = (index + 1) % pool->size();

        worker::Worker *worker = &pool->at(index);

        worker->queueTask(static_cast<void *>(&newTask));
    }

    job::TaskContainer<job::complete_tasks::CompleteTask, 128> *getCompleteMessages() noexcept
    {
        return m_completeTasks.get();
    }

    bool isThereWorkerForTask(const Handle &taskType) noexcept;

    worker::Worker *getWorker(const Handle &registeredTaskType) noexcept;

  private:
    absl::flat_hash_map<uint16_t, std::vector<worker::Worker>> m_workers;

    std::unique_ptr<job::TaskContainer<job::complete_tasks::CompleteTask, 128>> m_completeTasks = nullptr;

    absl::flat_hash_map<uint16_t, size_t> m_nextWorkerIndex;

    std::vector<worker::Worker> *getWorkerPool(const uint16_t &registeredType) noexcept;

    const std::vector<worker::Worker> *getWorkerPool(const uint16_t &registeredType) const noexcept;
};
} // namespace star::job