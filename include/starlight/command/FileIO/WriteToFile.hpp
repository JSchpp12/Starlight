#pragma once

#include "starlight/job/tasks/IOTask.hpp"

#include <star_common/IServiceCommand.hpp>

#include <string_view>

namespace star::command::file_io
{
namespace write_to_file
{
inline constexpr std::string_view GetWriteToFileCommandTypeName = "scwf";
}

struct WriteToFile : public common::IServiceCommand
{
    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return write_to_file::GetWriteToFileCommandTypeName;
    }

    WriteToFile(star::job::tasks::io::IOTask writeTask) : writeTask(std::move(writeTask))
    {
    }

    star::job::tasks::io::IOTask writeTask;
};
} // namespace star::command::file_io