#pragma once

#include <star_common/IServiceCommand.hpp>

namespace star::command_order
{
namespace trigger_pass
{
inline constexpr const char *GetTriggerPassCommandTypeName() noexcept
{
    return "coTrigPass";
};

} // namespace trigger_pass
struct TriggerPass : public star::common::IServiceCommand
{
    Handle passHandle;

    static inline constexpr const char *GetUniqueTypeName() noexcept
    {
        return trigger_pass::GetTriggerPassCommandTypeName();
    }

    TriggerPass(Handle passHandle) : star::common::IServiceCommand(), passHandle(std::move(passHandle))
    {
    }

    TriggerPass(uint16_t type, Handle passHandle)
        : star::common::IServiceCommand(std::move(type)), passHandle(std::move(passHandle))
    {
    }
};
} // namespace star::command_order
