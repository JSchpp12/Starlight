#include "starlight/job/tasks/IOTask.hpp"

#include "starlight/core/logging/LoggingFactory.hpp"

namespace star::job::tasks::io
{
void ExecuteIOTask(void *p)
{
    auto *payload = static_cast<IOPayload *>(p);

    if (!payload->writeFileFunction)
    {
        core::logging::warning("No write file function provided to IOTask. Skipping and attempting to continue");
        return;
    }

    payload->writeFileFunction(payload->filePath);
}

std::optional<star::job::complete_tasks::CompleteTask> CreateIOTaskComplete(void *p)
{
    return std::nullopt;
}

IOTask CreateIOTask(std::string filePath,
                    std::function<void(const std::string &)> writeFileFunction)
{
    return IOTask::Builder<IOPayload>()
        .setPayload(IOPayload{.filePath = std::move(filePath), .writeFileFunction = std::move(writeFileFunction)})
        .setExecute(&ExecuteIOTask)
        .build();
}
} // namespace star::job::tasks::io