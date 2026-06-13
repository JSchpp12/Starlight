#include "starlight/service/TaskSchedulerService.hpp"

namespace star::service
{
TaskSchedulerService::TaskSchedulerService() : m_listenForSubmitTask(*this)
{
}

TaskSchedulerService::TaskSchedulerService(TaskSchedulerService &&other)
    : m_taskManager(other.m_taskManager), m_cmdBus(other.m_cmdBus),
      m_listenForSubmitTask(*this)
{
    if (m_cmdBus != nullptr)
    {
        other.cleanupListeners(*m_cmdBus);
        initListeners(*m_cmdBus);
    }
}

TaskSchedulerService &TaskSchedulerService::operator=(TaskSchedulerService &&other)
{
    if (this != &other)
    {
        m_taskManager = other.m_taskManager;
        m_cmdBus = other.m_cmdBus;

        if (m_cmdBus != nullptr)
        {
            other.cleanupListeners(*m_cmdBus);
            initListeners(*m_cmdBus);
        }
    }

    return *this;
}

void TaskSchedulerService::init()
{
    assert(m_cmdBus != nullptr && "Command bus not saved from initParameters");

    initListeners(*m_cmdBus);
}

void TaskSchedulerService::negotiateWorkers(core::WorkerPool &pool, job::TaskManager &tm)
{
    (void)pool;
    m_taskManager = &tm;
}

void TaskSchedulerService::setInitParameters(InitParameters &params)
{
    m_cmdBus = &params.commandBus;
    params.taskScheduler = this;
}

void TaskSchedulerService::shutdown()
{
    assert(m_cmdBus != nullptr);

    cleanupListeners(*m_cmdBus);
}

void TaskSchedulerService::onSubmitTask(star::command::task_scheduler::SubmitTask &cmd)
{
    m_taskManager->submitTaskRoundRobin(std::move(cmd.task), cmd.taskTypeName);
}

job::TaskManager &TaskSchedulerService::getTaskManager()
{
    return *m_taskManager;
}

const job::TaskManager &TaskSchedulerService::getTaskManager() const
{
    return *m_taskManager;
}

void TaskSchedulerService::initListeners(core::CommandBus &bus)
{
    m_listenForSubmitTask.init(bus);
}

void TaskSchedulerService::cleanupListeners(core::CommandBus &bus)
{
    m_listenForSubmitTask.cleanup(bus);
}
} // namespace star::service