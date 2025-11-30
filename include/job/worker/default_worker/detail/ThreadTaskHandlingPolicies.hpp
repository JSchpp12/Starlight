#pragma once

#include "TaskContainer.hpp"
#include "logging/LoggingFactory.hpp"

#include <boost/atomic/atomic.hpp>
#include <boost/thread.hpp>

#include <sstream>

namespace star::job::worker::default_worker
{
template <typename TTask, size_t TQueueSize> class DefaultThreadTaskHandlingPolicy
{
  public:
    DefaultThreadTaskHandlingPolicy()
        : m_waitForWorkToFinishBeforeExiting(false), m_shouldRun(std::make_shared<boost::atomic<bool>>()),
          m_tasks(std::make_shared<job::TaskContainer<TTask, TQueueSize>>())
    {
    }
    explicit DefaultThreadTaskHandlingPolicy(bool waitForWorkToFinishBeforeExiting)
        : m_waitForWorkToFinishBeforeExiting(waitForWorkToFinishBeforeExiting),
          m_shouldRun(std::make_shared<boost::atomic<bool>>()),
          m_tasks(std::make_shared<job::TaskContainer<TTask, TQueueSize>>())
    {
    }

    void init(std::string workerName, TaskContainer<complete_tasks::CompleteTask, 128> *completeMessages)
    {
        m_workerName = std::make_shared<std::string>(workerName);
        m_completeMessages = completeMessages;

        startThread();
    }
    void queueTask(void *task)
    {
        TTask *typedTask = static_cast<TTask *>(task);
        m_tasks->queueTask(std::move(*typedTask));
    }
    void cleanup()
    {
        stopThread();
    }

  private:
    bool m_waitForWorkToFinishBeforeExiting = false;
    std::shared_ptr<boost::atomic<bool>> m_shouldRun = nullptr;
    std::shared_ptr<job::TaskContainer<TTask, TQueueSize>> m_tasks = nullptr;
    TaskContainer<complete_tasks::CompleteTask, 128> *m_completeMessages = nullptr;
    boost::thread thread = boost::thread();

    std::shared_ptr<std::string> m_workerName = nullptr;

    void startThread()
    {
        m_shouldRun->store(true);
        thread = boost::thread(&DefaultThreadTaskHandlingPolicy::threadFunction, &m_waitForWorkToFinishBeforeExiting,
                               m_shouldRun.get(), m_tasks.get(), m_completeMessages, m_workerName.get());
    }

    void stopThread()
    {
        m_shouldRun->store(false);
        if (thread.joinable())
            thread.join();
    }

    static void threadFunction(const bool *mustWaitForWorkToFinishBeforeExit, boost::atomic<bool> *shouldRun,
                               job::TaskContainer<TTask, TQueueSize> *tasks,
                               TaskContainer<complete_tasks::CompleteTask, 128> *completeMessages,
                               std::string *workerName)
    {
        assert(shouldRun != nullptr && tasks != nullptr && completeMessages != nullptr && workerName != nullptr);

        logStart(*workerName);

        bool run = shouldRun->load();
        while (run)
        {
            std::optional<TTask> task = tasks->getQueuedTask();

            if (task.has_value())
            {
                task.value().run();

                auto message = task.value().getCompleteMessage();
                if (message.has_value())
                {
                    completeMessages->queueTask(std::move(message.value()));
                }
            }
            else
            {
                boost::this_thread::sleep_for(boost::chrono::milliseconds(2));
            }

            if (!shouldRun->load() &&
                (!mustWaitForWorkToFinishBeforeExit || (mustWaitForWorkToFinishBeforeExit && !task.has_value())))
            {
                run = false;
            }
        }

        logStop(*workerName);
    }

    static void writeLog(std::string_view workerName, boost::log::trivial::severity_level level, std::string message)
    {
        std::ostringstream oss;
        oss << workerName << " - " << message;

        core::logging::log(std::move(level), oss.str());
    }

    static void logStart(std::string_view workerName)
    {
        writeLog(workerName, boost::log::trivial::info, "Beginning Work");
    }

    static void logStop(std::string_view workerName)
    {
        writeLog(workerName, boost::log::trivial::info, "Exiting");
    }
};
} // namespace star::job::worker::default_worker