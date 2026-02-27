#include "starlight/job/tasks/IOTask.hpp"

#include "starlight/core/logging/LoggingFactory.hpp"

#include <variant>

namespace star::job::tasks::io
{

std::optional<star::job::complete_tasks::CompleteTask> CreateWriteTaskComplete(void *p)
{
    return std::nullopt;
}

} // namespace star::job::tasks::io