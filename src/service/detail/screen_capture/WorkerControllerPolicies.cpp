#include "service/detail/screen_capture/WorkerControllerPolicies.hpp"

#include "logging/LoggingFactory.hpp"

namespace star::service::detail::screen_capture
{
void WorkerControllerPolicy::addWriteTask(star::job::tasks::write_image_to_disk::WriteImageTask newTask)
{
    core::logging::log(boost::log::trivial::info, "Adding write to disk task");
    void *t = static_cast<void *>(&newTask);
    // m_workers.front()->doQueueTask(t);
}
} // namespace star::service::detail::screen_capture