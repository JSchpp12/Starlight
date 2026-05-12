#pragma once

#include "BusyWaitTaskHandlingPolicy.hpp"
#include "starlight/core/logging/LoggingFactory.hpp"

namespace star::job::worker::default_worker
{
template <typename TTask, size_t TQueueSize>
class SleepWaitTaskHandlingPolicy : public BusyWaitTaskHandlingPolicy<TTask, TQueueSize>
{
  private:
    using Parent = BusyWaitTaskHandlingPolicy<TTask, TQueueSize>;

  public:
    SleepWaitTaskHandlingPolicy()
        : Parent(), m_taskMutex(std::make_unique<boost::mutex>()),
          m_hasTask(std::make_unique<boost::condition_variable>())
    {
    }
    explicit SleepWaitTaskHandlingPolicy(bool waitForWorkToFinishBeforeExiting)
        : Parent(waitForWorkToFinishBeforeExiting), m_taskMutex(std::make_unique<boost::mutex>()),
          m_hasTask(std::make_unique<boost::condition_variable>())
    {
    }
    SleepWaitTaskHandlingPolicy(const SleepWaitTaskHandlingPolicy &&) = delete;
    SleepWaitTaskHandlingPolicy &operator=(const SleepWaitTaskHandlingPolicy &&) = delete;
    SleepWaitTaskHandlingPolicy(SleepWaitTaskHandlingPolicy &&other) noexcept
        : Parent(std::move(other)), m_taskMutex(std::move(other.m_taskMutex)), m_hasTask(std::move(other.m_hasTask)) {};
    SleepWaitTaskHandlingPolicy &operator=(SleepWaitTaskHandlingPolicy &&other) noexcept
    {
        if (this != &other)
        {
            Parent::operator=(std::move(other));
            m_taskMutex = std::move(other.m_taskMutex);
            m_hasTask = std::move(other.m_hasTask);
        }
        return *this;
    };
    virtual ~SleepWaitTaskHandlingPolicy()
    {
        if (this->thread.joinable())
        {
            stopThread();
        }
    };

    virtual void stopThread() override
    {
        this->m_shouldRun->store(false);
        wakeupThread();
        if (this->thread.joinable())
        {
            this->thread.join();
        }
    }

    virtual void queueTask(void *task) override
    {
        {
            boost::lock_guard<boost::mutex> lock(*m_taskMutex);
            Parent::queueTask(task);
        }

        wakeupThread();
    }

  protected:
    std::unique_ptr<boost::mutex> m_taskMutex;
    std::unique_ptr<boost::condition_variable> m_hasTask;

    void wakeupThread()
    {
        m_hasTask->notify_one();
    }

    virtual void wait() override
    {
        core::logging::info("Entering wait");

        boost::unique_lock<boost::mutex> lock(*m_taskMutex);
        m_hasTask->wait(lock, [this] { return !this->m_shouldRun->load() || !this->m_tasks->empty(); });

        core::logging::info("Exiting wait");
    }
};
} // namespace star::job::worker::default_worker