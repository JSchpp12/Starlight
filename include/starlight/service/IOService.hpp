#pragma once

#include "job/worker/Worker.hpp"
#include "starlight/policy/command/ListenForWriteToFile.hpp"
#include "starlight/service/InitParameters.hpp"

namespace star::service
{
class IOService
{
    policy::ListenForWriteToFile<IOService> m_listenForFileWrite;
    job::worker::Worker *m_worker = nullptr;
    core::CommandBus *m_cmdBus = nullptr;

    void initListeners(star::core::CommandBus &bus);

    void cleanupListeners(star::core::CommandBus &bus);

  public:
    explicit IOService(job::worker::Worker *worker);
    IOService(const IOService &) = delete;
    IOService &operator=(const IOService &) = delete;
    IOService(IOService &&other); 
    IOService &operator=(IOService &&other); 
    ~IOService() = default;

    void init();

    void shutdown();

    void setInitParameters(InitParameters &params);

    void onWriteToFile(command::WriteToFile &event);
};
} // namespace star::service