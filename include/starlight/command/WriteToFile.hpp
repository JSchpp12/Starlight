#pragma once

#include <star_common/IServiceCommand.hpp>

#include <boost/filesystem/path.hpp>

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
    boost::filesystem::path m_path;
    std::function<void(const boost::filesystem::path &)> m_writeContentFunction;

    WriteToFile(boost::filesystem::path path,
                std::function<void(const boost::filesystem::path &)> writeContentFunction);

  public:
    struct Builder
    {
        Builder &setFile(boost::filesystem::path path);
        Builder &setWriteFileFunction(std::function<void(const boost::filesystem::path &)> writeFileFunction);
        WriteToFile build();

      private:
        boost::filesystem::path m_filePath;
        std::function<void(const boost::filesystem::path &)> m_writeFileFunction;
    };

    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return write_to_file::GetWriteToFileCommandTypeName;
    }

    boost::filesystem::path &getPath()
    {
        return m_path;
    }

    std::function<void(const boost::filesystem::path &)> &getFunction()
    {
        return m_writeContentFunction;
    }
};
} // namespace star::command