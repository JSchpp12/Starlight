#pragma once

#include "BusyWaitTaskHandlingPolicy.hpp"

namespace star::job::worker::default_worker
{
template <typename TTask, size_t TQueueSize> class SpinWaitTaskHandlingPolicy : public BusyWaitTaskHandlingPolicy<TTask, TQueueSize>
{
  private:
    using Parent = BusyWaitTaskHandlingPolicy<TTask, TQueueSize>;

  public:
    SpinWaitTaskHandlingPolicy() : Parent()
    {
    }
    explicit SpinWaitTaskHandlingPolicy(bool waitForWorkToFinishBeforeExiting)
        : Parent(waitForWorkToFinishBeforeExiting)
    {
    }
    SpinWaitTaskHandlingPolicy(const SpinWaitTaskHandlingPolicy &&) = delete;
    SpinWaitTaskHandlingPolicy &operator=(const SpinWaitTaskHandlingPolicy &&) = delete;
    SpinWaitTaskHandlingPolicy(SpinWaitTaskHandlingPolicy &&other) = default;
    SpinWaitTaskHandlingPolicy &operator=(SpinWaitTaskHandlingPolicy &&other) = default;
    ~SpinWaitTaskHandlingPolicy() = default;

  protected:
    void wait() override
    {
        boost::this_thread::yield();
    }
};
} // namespace star::job::worker::default_worker
