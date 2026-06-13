#pragma once

#include "InitParameters.hpp"
#include "starlight/command/TaskScheduler/SubmitTask.hpp"
#include "starlight/core/CommandBus.hpp"
#include "starlight/core/WorkerPool.hpp"
#include "job/TaskManager.hpp"
#include "job/worker/Worker.hpp"
#include "starlight/policy/command/ListenFor.hpp"

namespace star::service
{
template <typename T>
using ListenForSubmitTask = star::policy::command::ListenFor<
    T, star::command::task_scheduler::SubmitTask,
    star::command::task_scheduler::submit_task::GetSubmitTaskCommandTypeName, &T::onSubmitTask>;

class TaskSchedulerService
{
  public:
    TaskSchedulerService();
    TaskSchedulerService(const TaskSchedulerService &) = delete;
    TaskSchedulerService &operator=(const TaskSchedulerService &) = delete;
    TaskSchedulerService(TaskSchedulerService &&other);
    TaskSchedulerService &operator=(TaskSchedulerService &&other);
    ~TaskSchedulerService() = default;

    void init();

    void negotiateWorkers(core::WorkerPool &pool, job::TaskManager &tm);

    void setInitParameters(InitParameters &params);

    void shutdown();

    void onSubmitTask(star::command::task_scheduler::SubmitTask &cmd);

    job::TaskManager &getTaskManager();
    const job::TaskManager &getTaskManager() const;

  private:
    job::TaskManager *m_taskManager = nullptr;
    core::CommandBus *m_cmdBus = nullptr;
    ListenForSubmitTask<TaskSchedulerService> m_listenForSubmitTask;

    void initListeners(core::CommandBus &bus);
    void cleanupListeners(core::CommandBus &bus);
};
} // namespace star::service