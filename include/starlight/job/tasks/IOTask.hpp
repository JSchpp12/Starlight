#pragma once

#include "job/tasks/Task.hpp"

#include <boost/filesystem/path.hpp>

#include <functional>
#include <string_view>

namespace star::job::tasks::io
{
inline static constexpr std::string_view IOTaskName = "star::job::tasks::io"; 

struct IOPayload
{
    boost::filesystem::path filePath;
    std::function<void(const boost::filesystem::path &)> writeFileFunction;
};

using IOTask = star::job::tasks::Task<sizeof(IOPayload), alignof(IOPayload)>;

void ExecuteIOTask(void *p);

std::optional<star::job::complete_tasks::CompleteTask> CreateIOTaskComplete(void *p);

IOTask CreateIOTask(boost::filesystem::path filePath,
                    std::function<void(const boost::filesystem::path &)> writeFileFunction);

} // namespace star::job::tasks