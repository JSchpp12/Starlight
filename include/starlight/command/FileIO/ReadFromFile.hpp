#pragma

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

class ReadFromFile : public common::IServiceCommand
{
  public:
    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return read_from_file::GetReadFromFileCommandTypeName();
    }

    explicit ReadFromFile(star::job::tasks::io::IOTask readTask) : m_readTask(std::move(readTask))
    {
    }

    star::job::tasks::io::IOTask m_readTask;
};
} // namespace star::command::file_io