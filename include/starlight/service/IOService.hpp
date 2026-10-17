#pragma once

#include "starlight/command/FileIO/GetPath.hpp"
#include "starlight/command/FileIO/ReadFromFile.hpp"
#include "starlight/command/FileIO/WriteToFile.hpp"
#include "starlight/core/WorkerPool.hpp"
#include "starlight/job/tasks/IOTask.hpp"
#include "starlight/job/worker/DefaultWorker.hpp"
#include "starlight/job/worker/Worker.hpp"
#include "starlight/job/worker/detail/default_worker/SleepWaitTaskHandlingPolicy.hpp"
#include "starlight/policy/command/ListenFor.hpp"
#include "starlight/service/InitParameters.hpp"
#include "starlight/util/io/SearchPolicy.hpp"

#include <expected>

namespace star::service
{
template <typename T>
using ListenForReadFromFile =
    star::policy::command::ListenFor<T, star::command::file_io::ReadFromFile,
                                     star::command::file_io::read_from_file::GetReadFromFileCommandTypeName,
                                     &T::onReadFromFile>;
template <typename T>
using ListenForWriteToFile =
    star::policy::command::ListenFor<T, star::command::file_io::WriteToFile,
                                     star::command::file_io::write_to_file::GetWriteToFileCommandTypeName,
                                     &T::onWriteToFile>;
template <typename T>
using ListenForGetPath =
    star::policy::command::ListenFor<T, star::command::file_io::GetPath,
                                     star::command::file_io::get_path::GetGetPathCommandTypeName, &T::onGetPath>;
/// Owns the engine's file IO.
///
/// Reading and writing are queued to an IOWorker (see onReadFromFile /
/// onWriteToFile); *path resolution* is not. resolve() and the GetPath
/// command both run the search synchronously on the calling thread
class IOService
{
  public:
    explicit IOService(star::io::SearchPolicy policy = star::io::SearchPolicy{});
    IOService(const IOService &) = delete;
    IOService &operator=(const IOService &) = delete;
    IOService(IOService &&other);
    IOService &operator=(IOService &&other);
    ~IOService() = default;

    void init();

    void shutdown();

    void setInitParameters(InitParameters &params);

    void onReadFromFile(star::command::file_io::ReadFromFile &cmd);

    void onWriteToFile(command::file_io::WriteToFile &cmd);

    /// Handles GetPath submitted on the command bus; sets the reply
    /// synchronously. This is the entry point for callers that do not hold an
    /// IOService pointer (e.g. the terrain module, which only has the bus).
    void onGetPath(star::command::file_io::GetPath &cmd);

    /// Direct, synchronous resolution for callers that already hold this
    /// service (ShaderService, PipelineCommandService). Same semantics and same
    /// policy as the GetPath command above, minus the bus round-trip.
    std::expected<std::filesystem::path, star::io::IOError> resolve(const std::filesystem::path &candidate,
                                                                    star::io::PathKind kind) const;

    void negotiateWorkers(core::WorkerPool &pool, job::TaskManager &tm)
    {
        m_worker = worker(tm);
    }

  private:
    star::io::SearchPolicy m_policy;
    ListenForGetPath<IOService> m_listenForGetPath;
    ListenForReadFromFile<IOService> m_listenForReadFromFile;
    ListenForWriteToFile<IOService> m_listenForWriteToFile;
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