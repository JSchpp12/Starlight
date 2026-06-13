#pragma once

#include "starlight/job/tasks/Task.hpp"

#include <star_common/IServiceCommand.hpp>

#include <string_view>

namespace star::command::task_scheduler
{
namespace submit_task
{
inline constexpr const char *GetSubmitTaskCommandTypeName()
{
    return "tsST";
}
} // namespace submit_task

struct SubmitTask : public common::IServiceCommand
{
    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return submit_task::GetSubmitTaskCommandTypeName();
    }

    SubmitTask(job::tasks::Task<128> task, std::string_view taskTypeName)
        : common::IServiceCommand(), task(std::move(task)), taskTypeName(std::move(taskTypeName))
    {
    }

    SubmitTask(job::tasks::Task<128> task, std::string_view taskTypeName, uint16_t type)
        : common::IServiceCommand(type), task(std::move(task)), taskTypeName(std::move(taskTypeName))
    {
    }

    job::tasks::Task<128> task;
    std::string_view taskTypeName;
};
} // namespace star::command::task_scheduler