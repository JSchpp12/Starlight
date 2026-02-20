#pragma once

#include "starlight/command/FileIO/ReadFromFile.hpp"
#include "starlight/core/WorkerPool.hpp"
#include "starlight/job/tasks/IOTask.hpp"
#include "starlight/job/worker/DefaultWorker.hpp"
#include "starlight/job/worker/Worker.hpp"
#include "starlight/job/worker/detail/default_worker/SleepWaitTaskHandlingPolicy.hpp"
#include "starlight/policy/command/ListenFor.hpp"
#include "starlight/policy/command/ListenForWriteToFile.hpp"
#include "starlight/service/InitParameters.hpp"

namespace star::service
{
template <typename T>
using ListenForReadFromFile =
    star::policy::command::ListenFor<T, star::command::file_io::ReadFromFile,
                                     star::command::file_io::read_from_file::GetReadFromFileCommandTypeName,
                                     &T::onReadFromFile>;

class IOService
{
  public:
    IOService();
    IOService(const IOService &) = delete;
    IOService &operator=(const IOService &) = delete;
    IOService(IOService &&other);
    IOService &operator=(IOService &&other);
    ~IOService() = default;

    void init();

    void shutdown();

    void setInitParameters(InitParameters &params);

    void onReadFromFile(star::command::file_io::ReadFromFile &read);

    void onWriteToFile(command::file_io::WriteToFile &event);

    void negotiateWorkers(core::WorkerPool &pool, job::TaskManager &tm)
    {
        m_worker = worker(tm);
    }

  private:
    ListenForReadFromFile<IOService> m_listenForReadFromFile;
    policy::ListenForWriteToFile<IOService> m_listenForFileWrite;
    job::worker::Worker *m_worker = nullptr;
    core::CommandBus *m_cmdBus = nullptr;

    void initListeners(star::core::CommandBus &bus);

    void cleanupListeners(star::core::CommandBus &bus);

    job::worker::Worker *worker(job::TaskManager &tm) const
    {
        const std::string workerName = "IOWorker";
        Handle wHandle;
        {
            wHandle = tm.registerWorker(
                {star::job::worker::DefaultWorker(
                    job::worker::default_worker::SleepWaitTaskHandlingPolicy<job::tasks::io::IOTask, 64>{true},
                    workerName)},
                job::tasks::io::IOTaskName);
        }

        return tm.getWorker(wHandle);
    }
};
} // namespace star::service