#pragma once

#include "boost/atomic/atomic.hpp"

namespace star::Job
{
template <typename Payload> class Task
{
  public:
    enum class Status
    {
        Created,
        Submitted,
        Processing,
        Completed
    };

    Task(Payload data) : data(std::move(data))
    {
    }

    virtual ~Task() = default;

    virtual void execute()
    {
        this->currentStatus = Status::Processing;

        run(this->data);

        this->currentStatus = Status::Completed;
    }

    void setStatus(const Status &newStatus)
    {
        this->currentStatus = newStatus;
    }

    Status getStatus()
    {
        return this->currentStatus;
    }

  protected:
    virtual void run(Payload &payload) = 0;

  private:
    Payload data;
    Status currentStatus = Status::Created;
};
} // namespace star::Job