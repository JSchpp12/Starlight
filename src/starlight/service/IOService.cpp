#include "starlight/service/IOService.hpp"

#include "starlight/job/tasks/IOTask.hpp"

namespace star::service
{
IOService::IOService(star::io::SearchPolicy policy)
    : m_resolver(std::move(policy)), m_listenForGetPath(*this), m_listenForReadFromFile(*this),
      m_listenForWriteToFile(*this)
{
}

IOService::IOService(IOService &&other)
    : m_resolver(std::move(other.m_resolver)), m_listenForGetPath(*this), m_listenForReadFromFile(*this),
      m_listenForWriteToFile(*this), m_worker(other.m_worker), m_cmdBus(other.m_cmdBus)
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
        m_resolver = std::move(other.m_resolver);
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
    m_listenForGetPath.init(bus);
    m_listenForReadFromFile.init(bus);
    m_listenForWriteToFile.init(bus);
}

void IOService::cleanupListeners(star::core::CommandBus &bus)
{
    m_listenForGetPath.cleanup(bus);
    m_listenForReadFromFile.cleanup(bus);
    m_listenForWriteToFile.cleanup(bus);
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
    // Publish this service to later services (ShaderService,
    // PipelineCommandService) that funnel path resolution through it. Same
    // pattern as TaskSchedulerService: services are parameterized in list
    // order, so IOService must precede its consumers (guaranteed by
    // EngineServicesFactory::createDefault).
    params.ioService = this;
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

void IOService::onGetPath(star::command::file_io::GetPath &cmd)
{
    cmd.getReply().set(m_resolver.resolve(cmd.getCandidate(), cmd.getKind()));
}

std::expected<std::filesystem::path, star::io::IOError> IOService::resolve(const std::filesystem::path &candidate,
                                                                           star::io::PathKind kind) const
{
    return m_resolver.resolve(candidate, kind);
}

} // namespace star::service