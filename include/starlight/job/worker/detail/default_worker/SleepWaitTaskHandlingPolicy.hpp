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
        : m_taskMutex(std::move(other.m_taskMutex)), m_hasTask(std::move(other.m_hasTask)) {};
    SleepWaitTaskHandlingPolicy &operator=(SleepWaitTaskHandlingPolicy &&other) noexcept
    {
        if (this != &other)
        {
            m_taskMutex = std::move(other.m_taskMutex);
            m_hasTask = std::move(other.m_hasTask);

            // might need to check to stop/start child thread?
        }
        return *this;
    };
    virtual ~SleepWaitTaskHandlingPolicy() = default;

    virtual void queueTask(void *task) override
    {
        Parent::queueTask(task);
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
        boost::unique_lock<boost::mutex> lock(*m_taskMutex);
        m_hasTask->wait_for(lock, boost::chrono::milliseconds(100));
    }
};
} // namespace star::job::worker::default_worker