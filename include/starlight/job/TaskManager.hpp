#pragma once

#include "FrameScheduler.hpp"
#include "complete_tasks/CompleteTask.hpp"
#include "job/worker/Worker.hpp"

#include <star_common/Handle.hpp>
#include <star_common/HandleTypeRegistry.hpp>

#include <memory>
#include <typeindex>
#include <absl/container/flat_hash_map.h>

namespace star::job
{
class TaskManager
{
  public:
    TaskManager() : m_completeTasks(std::make_unique<job::TaskContainer<job::complete_tasks::CompleteTask, 128>>()) {};
    ~TaskManager() = default;

    TaskManager(const TaskManager &) = delete;
    TaskManager &operator=(const TaskManager &) = delete;

    TaskManager(TaskManager &&) = default;
    TaskManager &operator=(TaskManager &&) = default;

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
            throw std::runtime_error("Failed to find corresponding worker for the provided task type");
        }

        TTask *storedTask = new TTask(std::move(newTask));
        worker->queueTask(static_cast<void *>(storedTask));
        delete storedTask;
    }

    template <typename TTask> void submitTask(TTask &&newTask, std::string_view taskName)
    {
        Handle handle = {.type = common::HandleTypeRegistry::instance().getTypeGuaranteedExist(taskName), .id = 0};

        submitTask<TTask>(std::forward<TTask>(newTask), handle);
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

    std::vector<worker::Worker> *getWorkerPool(const uint16_t &registeredType) noexcept;

    const std::vector<worker::Worker> *getWorkerPool(const uint16_t &registeredType) const noexcept;
};
} // namespace star::job