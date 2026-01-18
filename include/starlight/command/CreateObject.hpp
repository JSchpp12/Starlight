#pragma once

#include "starlight/virtual/StarObject.hpp"

#include <star_common/IServiceCommand.hpp>
#include <star_common/ServiceReply.hpp>
#include <string_view>

namespace star::command
{
namespace create_object
{
inline constexpr std::string_view GetCreateObjectCommandTypeName = "star::common::create_object";
}

struct CreateObject : public common::IServiceCommand
{
    struct Builder
    {
        Builder &setParentDir(std::string parentDir);
        Builder &setFileName(std::string fileName);
        CreateObject build();

      private:
        std::string m_parentDir;
        std::string m_fileName;
    };

    std::string parentDir;
    std::string fileName;
    common::ServiceReply<std::unique_ptr<star::StarObject>> object;

    static constexpr std::string_view getName()
    {
        return create_object::GetCreateObjectCommandTypeName;
    }

    CreateObject(std::string parentDir, std::string fileName);
};
} // namespace star::command