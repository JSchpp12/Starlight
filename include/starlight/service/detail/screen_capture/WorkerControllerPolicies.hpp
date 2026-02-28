#pragma once

#include "job/TaskManager.hpp"
#include "job/tasks/WriteImageToDisk.hpp"
#include "job/worker/Worker.hpp"


namespace star::service::detail::screen_capture
{
class WorkerControllerPolicy
{
  public:

    void init(std::vector<job::worker::Worker::WorkerConcept *> workers); 

    void addWriteTask(star::job::tasks::write_image_to_disk::WriteImageTask newTask);

  private:
    std::vector<job::worker::Worker::WorkerConcept *> m_workers;
    size_t m_nextWorkerIndexToUse = 0; 

    void selectNextWorker();
};
} // namespace star::service::detail::screen_capture