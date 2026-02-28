#pragma once

#include <star_common/Handle.hpp>
#include <star_common/IServiceCommand.hpp>

#include <string_view>

namespace star::command_order
{

namespace declare_dependency
{
inline constexpr const char *GetDeclareDependencyCommandTypeName()
{
    return "coDecDep";
}
} // namespace declare_dependency

class DeclareDependency : public common::IServiceCommand
{
  public:
    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return declare_dependency::GetDeclareDependencyCommandTypeName();
    }

    DeclareDependency(uint16_t type, Handle src, Handle dep)
        : common::IServiceCommand(std::move(type)), m_src(std::move(src)), m_dep(std::move(dep))
    {
    }

    DeclareDependency(Handle src, Handle dep) : m_src(std::move(src)), m_dep(std::move(dep))
    {
    }

    const Handle &getSrc() const
    {
        return m_src;
    }
    const Handle &getDep() const
    {
        return m_dep;
    }

  private:
    Handle m_src;
    Handle m_dep;
};
} // namespace star::command_order