#pragma once

#include "starlight/ShaderResolver.hpp"
#include "starlight/command/detail/create_object/ObjectLoader.hpp"
#include "starlight/object/StarObject.hpp"

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
        Builder &setShaderResolver(ShaderResolver resolver);
        CreateObject build();

      private:
        std::string m_uniqueName = std::string();
        std::unique_ptr<create_object::ObjectLoader> m_loader = nullptr;
        ShaderResolver m_resolver{};
        bool m_resolverSet = false;
    };

    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return create_object::GetCreateObjectCommandTypeName;
    }

    CreateObject(std::string uniqueName, std::unique_ptr<create_object::ObjectLoader> loader,
                 ShaderResolver shaderResolver)
        : m_uniqueName(std::move(uniqueName)), m_loader(std::move(loader)),
          m_shaderResolver(std::move(shaderResolver))
    {
        assert(!m_uniqueName.empty());
        assert(m_loader != nullptr);
    }

    std::shared_ptr<StarObject> load()
    {
        return m_loader->load(m_shaderResolver);
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
    ShaderResolver m_shaderResolver;
};
} // namespace star::command