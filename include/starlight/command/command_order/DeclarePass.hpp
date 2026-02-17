#pragma once

#include <star_common/Handle.hpp>
#include <star_common/IServiceCommand.hpp>

namespace star::command_order
{
namespace declare_pass
{
inline constexpr const char *GetDeclarePassCommandTypeName()
{
    return "DeclarePass";
}
} // namespace declare_pass

class DeclarePass : public common::IServiceCommand
{
  public:
    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return declare_pass::GetDeclarePassCommandTypeName();
    }
    
    DeclarePass(Handle passHandle, uint32_t queueFamily)
        : m_passHandle(std::move(passHandle)), m_queueFamily(queueFamily)
    {
    }

    DeclarePass(uint16_t type, Handle passHandle, uint32_t queueFamily)
        : common::IServiceCommand(std::move(type)), m_passHandle(std::move(passHandle)), m_queueFamily(queueFamily)
    {
    }

    const Handle &getPassHandle() const
    {
        return m_passHandle;
    }

    uint32_t getQueueFamily() const
    {
        return m_queueFamily;
    }

  private:
    Handle m_passHandle;
    uint32_t m_queueFamily;
};
} // namespace star::command_order