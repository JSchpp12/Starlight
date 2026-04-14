#pragma once

#include "starlight/common/entities/Light.hpp"

#include <star_common/IServiceCommandWithReply.hpp>

namespace star::command
{
namespace create_light
{
enum SceneAddResult
{
    success,
    fail
};

inline constexpr const char *GetUniqueTypeName()
{
    return "ScCl";
};
} // namespace create_light

struct CreateLight : public star::common::IServiceCommandWithReply<std::pair<create_light::SceneAddResult, std::shared_ptr<std::vector<Light>>>>
{
    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return create_light::GetUniqueTypeName();
    }

    CreateLight &setName(std::string name)
    {
        m_name = std::move(name);
        return *this;
    }

    const std::string &getName() const
    {
        return m_name;
    }

  private:
    std::string m_name;
};
} // namespace star::command