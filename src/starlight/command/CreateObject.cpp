#include "starlight/command/CreateObject.hpp"

namespace star::command
{
CreateObject::Builder &CreateObject::Builder::setParentDir(std::string parentDir)
{
    m_parentDir = std::move(parentDir);
    return *this;
}

CreateObject::Builder &CreateObject::Builder::setFileName(std::string fileName)
{
    m_fileName = std::move(fileName);
    return *this;
}

CreateObject CreateObject::Builder::build()
{
    return {std::move(m_parentDir), std::move(m_fileName)};
}

CreateObject::CreateObject(std::string parentDir, std::string fileName)
    : common::IServiceCommand(), parentDir(std::move(parentDir)), fileName(std::move(fileName))
{
}
} // namespace star::command