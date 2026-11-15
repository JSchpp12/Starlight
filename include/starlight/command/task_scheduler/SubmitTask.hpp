#pragma once

#include "starlight/job/tasks/Task.hpp"

#include <star_common/IServiceCommand.hpp>

#include <array>

#include <string_view>

namespace star::command::task_scheduler
{
namespace submit_task
{
inline constexpr const char *GetSubmitTaskCommandTypeName()
{
    return "tsST";
}

inline constexpr const char *GetSubmitMultipleTasksCommandTypeName()
{
    return "tsSTs";
}
} // namespace submit_task

struct SubmitTask : public common::IServiceCommand
{
    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return submit_task::GetSubmitTaskCommandTypeName();
    }

    explicit SubmitTask(job::tasks::Task<> t) : common::IServiceCommand(), task({std::move(t)})
    {
    }

    job::tasks::Task<> task;
};

struct SubmitMultipleTasks : public common::IServiceCommand
{
    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return submit_task::GetSubmitMultipleTasksCommandTypeName();
    }
    static constexpr std::size_t k_max = 4;

    bool add(job::tasks::Task<> t) noexcept
    {
        if (count < k_max)
        {
            tasks[count++] = std::move(t);
            return true;
        }
        return false;
    }

    std::array<job::tasks::Task<>, k_max> tasks;
    size_t count{0};
};

} // namespace star::command::task_scheduler