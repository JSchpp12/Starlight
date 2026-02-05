#include "starlight/command/WriteToFile.hpp"

namespace star::command
{
WriteToFile::Builder &WriteToFile::Builder::setFile(std::string path)
{
    m_path = std::move(path);
    return *this;
}
WriteToFile::Builder &WriteToFile::Builder::setWriteFileFunction(
    std::function<void(const std::string &)> writeFileFunction)
{
    m_writeFileFunction = std::move(writeFileFunction);
    return *this;
}
WriteToFile WriteToFile::Builder::build()
{
    return WriteToFile{std::move(m_path), std::move(m_writeFileFunction)};
}

WriteToFile::WriteToFile(std::string path,
                         std::function<void(const std::string &)> writeContentFunction)
    : m_path(std::move(path)), m_writeContentFunction(std::move(writeContentFunction))
{
}

} // namespace star::command