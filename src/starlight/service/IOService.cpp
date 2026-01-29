#include "starlight/service/IOService.hpp"

#include "starlight/job/tasks/IOTask.hpp"

namespace star::service
{
IOService::IOService(job::worker::Worker *worker) : m_listenForFileWrite(*this), m_worker(worker)
{
}

IOService::IOService(IOService &&other)
    : m_listenForFileWrite(*this), m_worker(other.m_worker), m_cmdBus(other.m_cmdBus)
{
    if (m_cmdBus != nullptr)
    {
        other.cleanupListeners(*m_cmdBus);

        initListeners(*m_cmdBus);
    }
}

IOService &IOService::operator=(IOService &&other)
{
    if (this != &other)
    {
        m_worker = other.m_worker;
        m_cmdBus = other.m_cmdBus;

        if (m_cmdBus != nullptr)
        {
            other.cleanupListeners(*m_cmdBus);
            initListeners(*m_cmdBus);
        }
    }

    return *this;
}

void IOService::initListeners(star::core::CommandBus &bus)
{
    m_listenForFileWrite.init(bus);
}

void IOService::cleanupListeners(star::core::CommandBus &bus)
{
    m_listenForFileWrite.cleanup(bus);
}

void IOService::init()
{
    assert(m_cmdBus != nullptr && "Command bus not saved from initParameters");

    initListeners(*m_cmdBus);
}

void IOService::shutdown()
{
    assert(m_cmdBus != nullptr);

    cleanupListeners(*m_cmdBus);
}

void IOService::setInitParameters(InitParameters &params)
{
    m_cmdBus = &params.commandBus;
}

void IOService::onWriteToFile(command::WriteToFile &event)
{
    assert(m_worker != nullptr);

    // create the event and dispatch to worker
    job::tasks::io::IOTask *task =
        new job::tasks::io::IOTask(star::job::tasks::io::CreateIOTask(event.getPath(), event.getFunction()));
    m_worker->queueTask(task);
    delete task;
}

} // namespace star::service