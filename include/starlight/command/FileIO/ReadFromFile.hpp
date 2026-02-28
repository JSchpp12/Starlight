#pragma once

#include "starlight/job/tasks/IOTask.hpp"

#include <star_common/IServiceCommandWithReply.hpp>

#include <functional>
#include <string_view>

namespace star::command::file_io
{
namespace read_from_file
{
inline constexpr const char *GetReadFromFileCommandTypeName()
{
    return "scrff";
}
} // namespace read_from_file

struct ReadFromFile : public common::IServiceCommand
{
    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return read_from_file::GetReadFromFileCommandTypeName();
    }

    explicit ReadFromFile(star::job::tasks::io::IOTask readTask)
        : common::IServiceCommand(), readTask(std::move(readTask))
    {
    }

    ReadFromFile(star::job::tasks::io::IOTask readTask, uint16_t type)
        : common::IServiceCommand(type), readTask(std::move(readTask))
    {
    }

    star::job::tasks::io::IOTask readTask;
};
} // namespace star::command::file_io