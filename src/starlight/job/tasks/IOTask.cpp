#include "starlight/job/tasks/IOTask.hpp"

namespace star::job::tasks::io
{
void ExecuteIOTask(void *p)
{
}

std::optional<star::job::complete_tasks::CompleteTask> CreateIOTaskComplete(void *p)
{
    return std::nullopt;
}

IOTask CreateIOTask(boost::filesystem::path filePath,
                    std::function<void(const boost::filesystem::path &)> writeFileFunction)
{
    return IOTask::Builder<IOPayload>()
    .setPayload(IOPayload{
        .filePath = std::move(filePath),
        .writeFileFunction = std::move(writeFileFunction)
    })
    .setExecute(&ExecuteIOTask)
    .build();
}
} // namespace star::job::tasks