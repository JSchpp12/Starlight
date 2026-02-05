#pragma once

#include "job/tasks/Task.hpp"

#include <functional>
#include <string_view>

namespace star::job::tasks::io
{
inline static constexpr std::string_view IOTaskName = "star::job::tasks::io"; 

struct IOPayload
{
    std::string filePath;
    std::function<void(const std::string &)> writeFileFunction;
};

using IOTask = star::job::tasks::Task<sizeof(IOPayload), alignof(IOPayload)>;

void ExecuteIOTask(void *p);

std::optional<star::job::complete_tasks::CompleteTask> CreateIOTaskComplete(void *p);

IOTask CreateIOTask(std::string filePath,
                    std::function<void(const std::string &)> writeFileFunction);

} // namespace star::job::tasks