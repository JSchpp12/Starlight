#pragma once

#include "tasks/Task.hpp"
#include "complete_tasks/CompleteTask.hpp"
#include <boost/lockfree/stack.hpp>

namespace star::job::worker
{
class WorkerBase
{
  public:
    virtual ~WorkerBase() = default;
    virtual void stop() = 0;
    virtual void start() = 0;
    virtual void queueTask(tasks::Task<> task) = 0;
    virtual void setCompleteMessageCommunicationStructure(
        boost::lockfree::stack<complete_tasks::CompleteTask<>, boost::lockfree::capacity<128>> *completeMessages) = 0;
    // Add any other operations you need.
};
} // namespace star::job::worker