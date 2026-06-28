#include "starlight/job/tasks/TransferTask.hpp"

#include "starlight/core/logging/LoggingFactory.hpp"

#include <cassert>

namespace star::job::tasks::transfer
{

std::optional<star::job::complete_tasks::CompleteTask> CreateTransferComplete(void *p)
{
    (void)p;
    return std::nullopt;
}

void ExecuteTransferTask(void *p)
{
    (void)p;
    // Transfer tasks are processed by the dedicated transfer worker policy, which owns the
    // Vulkan queue/allocator state. This function is required by the Task type system but
    // should not be invoked directly outside of that policy.
    core::logging::warning("ExecuteTransferTask invoked outside of transfer worker policy");
}

} // namespace star::job::tasks::transfer
