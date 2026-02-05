#pragma once

#include <star_common/IServiceCommand.hpp>

#include <string_view>

namespace star::command
{
namespace write_to_file
{
inline constexpr std::string_view GetWriteToFileCommandTypeName = "star::command::write_to_file";
}

class WriteToFile : public common::IServiceCommand
{
  private:
    std::string m_path;
    std::function<void(const std::string &)> m_writeContentFunction;

    WriteToFile(std::string path,
                std::function<void(const std::string &)> writeContentFunction);

  public:
    struct Builder
    {
        Builder &setFile(std::string path);
        Builder &setWriteFileFunction(std::function<void(const std::string &)> writeFileFunction);
        WriteToFile build();

      private:
        std::string m_path; 
        std::function<void(const std::string &)> m_writeFileFunction;
    };

    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return write_to_file::GetWriteToFileCommandTypeName;
    }

    const std::string &getPath()
    {
        return m_path;
    }

    std::function<void(const std::string &)> &getFunction()
    {
        return m_writeContentFunction;
    }
};
} // namespace star::command