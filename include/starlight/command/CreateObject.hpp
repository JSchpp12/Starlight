#pragma once

#include "starlight/command/detail/create_object/ObjectLoader.hpp"
#include "starlight/virtual/StarObject.hpp"

#include <star_common/IServiceCommand.hpp>
#include <star_common/ServiceReply.hpp>

#include <string_view>

namespace star::command
{
namespace create_object
{
inline constexpr std::string_view GetCreateObjectCommandTypeName = "star::command::create_object";
}

struct CreateObject : public common::IServiceCommand
{
    struct Builder
    {
        Builder &setLoader(std::unique_ptr<create_object::ObjectLoader> loader);
        Builder &setUniqueName(std::string name);
        CreateObject build();

      private:
        std::string m_uniqueName = std::string();
        std::unique_ptr<create_object::ObjectLoader> m_loader = nullptr;
    };

    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return create_object::GetCreateObjectCommandTypeName;
    }

    CreateObject(std::string uniqueName, std::unique_ptr<create_object::ObjectLoader> loader)
        : m_uniqueName(std::move(uniqueName)), m_loader(std::move(loader))
    {
        assert(!m_uniqueName.empty());
        assert(m_loader != nullptr);
    }

    std::shared_ptr<StarObject> load()
    {
        return m_loader->load();
    }

    const std::string &getUniqueName() const
    {
        return m_uniqueName;
    }

    common::ServiceReply<std::shared_ptr<star::StarObject>> &getReply()
    {
        return result;
    }

  private:
    std::string m_uniqueName;
    common::ServiceReply<std::shared_ptr<star::StarObject>> result;
    std::unique_ptr<create_object::ObjectLoader> m_loader = nullptr;
};
} // namespace star::command