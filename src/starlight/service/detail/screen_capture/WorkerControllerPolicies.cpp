#include "service/detail/screen_capture/WorkerControllerPolicies.hpp"

#include "logging/LoggingFactory.hpp"

namespace star::service::detail::screen_capture
{
void WorkerControllerPolicy::addWriteTask(star::job::tasks::write_image_to_disk::WriteImageTask newTask)
{
    selectNextWorker();
    
    void *t = static_cast<void *>(&newTask);
    m_workers[m_nextWorkerIndexToUse]->doQueueTask(t);
}

void WorkerControllerPolicy::selectNextWorker()
{
    m_nextWorkerIndexToUse++;
    if (m_nextWorkerIndexToUse >= m_workers.size()){
        m_nextWorkerIndexToUse = 0;
    }
}
} // namespace star::service::detail::screen_capture