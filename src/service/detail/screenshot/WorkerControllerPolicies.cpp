#include "service/detail/screenshot/WorkerControllerPolicies.hpp"

#include "logging/LoggingFactory.hpp"

namespace star::service::detail::screenshot
{
void WorkerControllerPolicy::addWriteTask()
{
    core::logging::log(boost::log::trivial::info, "Adding write to disk task");
}
} // namespace star::service::detail::screenshot