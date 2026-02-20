#include "starlight/job/tasks/IOTask.hpp"

#include "starlight/core/logging/LoggingFactory.hpp"

#include <variant>

namespace star::job::tasks::io
{

void ExecuteWriteTask(void *p)
{
    auto *payload = static_cast<WritePayload *>(p);

    if (!payload->writeFileFunction)
    {
        core::logging::warning("No write file function provided to IOTask. Skipping and attempting to continue");
        return;
    }

    payload->writeFileFunction(payload->filePath);
}

std::optional<star::job::complete_tasks::CompleteTask> CreateWriteTaskComplete(void *p)
{
    return std::nullopt;
}

IOTask CreateIOTask(std::string filePath,
                    std::function<void(const std::string &)> writeFileFunction)
{
    return IOTask::Builder<WritePayload>()
        .setPayload(WritePayload{.filePath = std::move(filePath), .writeFileFunction = std::move(writeFileFunction)})
        .setExecute(&ExecuteWriteTask)
        .build();
}

} // namespace star::job::tasks::io