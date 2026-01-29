#include "starlight/command/WriteToFile.hpp"

namespace star::command
{
WriteToFile::Builder &WriteToFile::Builder::setFile(boost::filesystem::path path)
{
    m_filePath = std::move(path);
    return *this;
}
WriteToFile::Builder &WriteToFile::Builder::setWriteFileFunction(
    std::function<void(const boost::filesystem::path &)> writeFileFunction)
{
    m_writeFileFunction = std::move(writeFileFunction);
    return *this;
}
WriteToFile WriteToFile::Builder::build()
{
    return WriteToFile{std::move(m_filePath), std::move(m_writeFileFunction)};
}

WriteToFile::WriteToFile(boost::filesystem::path path,
                         std::function<void(const boost::filesystem::path &)> writeContentFunction)
    : m_path(std::move(path)), m_writeContentFunction(std::move(writeContentFunction))
{
}

} // namespace star::command