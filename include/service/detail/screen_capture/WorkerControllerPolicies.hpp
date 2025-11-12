#pragma once

#include "job/TaskManager.hpp"
#include "job/tasks/WriteImageToDisk.hpp"
#include "job/worker/Worker.hpp"


namespace star::service::detail::screen_capture
{
class WorkerControllerPolicy
{
  public:
    WorkerControllerPolicy(std::vector<job::worker::Worker::WorkerConcept *> workers) : m_workers(std::move(workers))
    {
    }
    void addWriteTask();

  private:
    std::vector<job::worker::Worker::WorkerConcept *> m_workers;
};
} // namespace star::service::detail::screen_capture