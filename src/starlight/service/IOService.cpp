#include "starlight/service/IOService.hpp"

#include "starlight/job/tasks/IOTask.hpp"

namespace star::service
{
IOService::IOService() : m_listenForReadFromFile(*this), m_listenForFileWrite(*this)
{
}

IOService::IOService(IOService &&other)
    : m_listenForReadFromFile(*this), m_listenForFileWrite(*this), m_worker(other.m_worker), m_cmdBus(other.m_cmdBus)
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
    m_listenForReadFromFile.init(bus);
    m_listenForFileWrite.init(bus);
}

void IOService::cleanupListeners(star::core::CommandBus &bus)
{
    m_listenForReadFromFile.cleanup(bus);
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

void IOService::onReadFromFile(command::file_io::ReadFromFile &cmd)
{
    assert(m_worker != nullptr);

    m_worker->queueTask(&cmd.readTask);
}

void IOService::onWriteToFile(command::file_io::WriteToFile &cmd)
{
    assert(m_worker != nullptr);

    // create the event and dispatch to worker
    m_worker->queueTask(&cmd.writeTask);
}

} // namespace star::service