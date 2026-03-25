#pragma once

#include "starlight/core/exceptions.hpp"
#include "starlight/service/detail/command_order/TriggerDescription.hpp"

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
    star::service::command_order::TriggerDescription description;

    static inline constexpr const char *GetUniqueTypeName() noexcept
    {
        return trigger_pass::GetTriggerPassCommandTypeName();
    }
    TriggerPass() = default;
    TriggerPass(uint16_t type) : star::common::IServiceCommand(std::move(type)), description()
    {
    }

    TriggerPass &setPass(Handle handle)
    {
        description.passDefinition = std::move(handle);
        return *this;
    }
    TriggerPass &setBinarySemaphore(vk::Semaphore semaphore)
    {
        if (std::holds_alternative<star::service::command_order::TriggerDescription::BinarySemaphore>(
                description.semaphoreInfo))
        {
            std::get<star::service::command_order::TriggerDescription::BinarySemaphore>(description.semaphoreInfo)
                .semaphore = std::move(semaphore);
        }
        else
        {
            STAR_THROW("This command has already been provided a timeline semaphore");
        }
        return *this;
    }
    TriggerPass &setTimelineSemaphore(Handle handle)
    {
        assert(handle.isInitialized() && "Unintialized handle provided");

        if (std::holds_alternative<star::service::command_order::TriggerDescription::BinarySemaphore>(
                description.semaphoreInfo))
        {
            description.semaphoreInfo = star::service::command_order::TriggerDescription::TimelineSemaphore{};
        }

        std::get<star::service::command_order::TriggerDescription::TimelineSemaphore>(description.semaphoreInfo)
            .record = std::move(handle);

        return *this;
    }
    TriggerPass &setSignalValue(uint64_t signalValue)
    {
        if (std::holds_alternative<star::service::command_order::TriggerDescription::BinarySemaphore>(
                description.semaphoreInfo))
        {
            description.semaphoreInfo = star::service::command_order::TriggerDescription::TimelineSemaphore{};
        }

        std::get<star::service::command_order::TriggerDescription::TimelineSemaphore>(description.semaphoreInfo)
            .signalValue = std::move(signalValue);
        return *this;
    }
};
} // namespace star::command_order
