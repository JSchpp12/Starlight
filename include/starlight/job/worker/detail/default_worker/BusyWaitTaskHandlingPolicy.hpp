#pragma once

#include "TaskContainer.hpp"
#include "logging/LoggingFactory.hpp"

#include <boost/atomic/atomic.hpp>
#include <boost/thread.hpp>

#include <sstream>

namespace star::job::worker::default_worker
{
template <typename TTask, size_t TQueueSize> class BusyWaitTaskHandlingPolicy
{
  public:
    BusyWaitTaskHandlingPolicy()
        : m_waitForWorkToFinishBeforeExiting(false), m_shouldRun(std::make_shared<boost::atomic<bool>>()),
          m_tasks(std::make_shared<job::TaskContainer<TTask, TQueueSize>>())
    {
    }

    explicit BusyWaitTaskHandlingPolicy(bool waitForWorkToFinishBeforeExiting)
        : m_waitForWorkToFinishBeforeExiting(waitForWorkToFinishBeforeExiting),
          m_shouldRun(std::make_shared<boost::atomic<bool>>()),
          m_tasks(std::make_shared<job::TaskContainer<TTask, TQueueSize>>())
    {
    }
    BusyWaitTaskHandlingPolicy(const BusyWaitTaskHandlingPolicy &&) = delete;
    BusyWaitTaskHandlingPolicy &operator=(const BusyWaitTaskHandlingPolicy &&) = delete;
    BusyWaitTaskHandlingPolicy(BusyWaitTaskHandlingPolicy &&) = default;
    BusyWaitTaskHandlingPolicy &operator=(BusyWaitTaskHandlingPolicy &&) = default;
    virtual ~BusyWaitTaskHandlingPolicy() = default;

    void init(std::string workerName, TaskContainer<complete_tasks::CompleteTask, 128> *completeMessages)
    {
        m_workerName = std::move(workerName);
        m_completeMessages = completeMessages;

        startThread();
    }

    virtual void queueTask(void *task)
    {
        TTask *typedTask = static_cast<TTask *>(task);
        m_tasks->queueTask(std::move(*typedTask));
    }
    void cleanup()
    {
        stopThread();
    }

  protected:
    bool m_waitForWorkToFinishBeforeExiting = false;
    std::shared_ptr<boost::atomic<bool>> m_shouldRun = nullptr;
    std::shared_ptr<job::TaskContainer<TTask, TQueueSize>> m_tasks = nullptr;
    TaskContainer<complete_tasks::CompleteTask, 128> *m_completeMessages = nullptr;
    boost::thread thread;
    std::string m_workerName;

    void startThread()
    {
        m_shouldRun->store(true);
        thread = boost::thread([this]() { threadFunction(); });
    }

    void stopThread()
    {
        m_shouldRun->store(false);
        if (thread.joinable())
            thread.join();
    }

    virtual void wait()
    {
        boost::this_thread::sleep_for(boost::chrono::milliseconds(2));
    }

    void threadFunction()
    {
        logStart(m_workerName);

        bool run = m_shouldRun->load();
        while (run)
        {
            std::optional<TTask> task = m_tasks->getQueuedTask();

            if (task.has_value())
            {
                task.value().run();

                auto message = task.value().getCompleteMessage();
                if (message.has_value())
                {
                    m_completeMessages->queueTask(std::move(message.value()));
                }
            }
            else
            {
                wait();
            }

            if (!m_shouldRun->load() &&
                (!m_waitForWorkToFinishBeforeExiting || (m_waitForWorkToFinishBeforeExiting && m_tasks->empty())))
            {
                run = false;
            }
        }

        logStop(m_workerName);
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