#pragma once

#include "starlight/service/detail/command_order/EdgeDescription.hpp"

#include <star_common/Handle.hpp>
#include <star_common/IServiceCommandWithReply.hpp>

namespace star::command_order
{
namespace get_pass_info
{

inline constexpr const char *GetPassInfoTypeName()
{
    return "coPI";
}

struct GatheredPassInfo
{
    vk::Semaphore signaledSemaphore;
    uint64_t toSignalValue{0};
    uint64_t currentSignalValue{0};

    bool isTriggeredThisFrame = false;
    const uint32_t *queueFamilyIndex = nullptr;
    const std::vector<bool> *wasProcessedOnLastFrame = nullptr;
    const std::vector<star::service::command_order::EdgeDescription> *edges = nullptr;
};
} // namespace get_pass_info

struct GetPassInfo : public star::common::IServiceCommandWithReply<get_pass_info::GatheredPassInfo>
{
    static std::string_view GetUniqueTypeName()
    {
        return get_pass_info::GetPassInfoTypeName();
    }

    GetPassInfo(uint16_t type, const Handle &pass)
        : star::common::IServiceCommandWithReply<get_pass_info::GatheredPassInfo>(std::move(type)), pass(&pass) {};
    explicit GetPassInfo(const Handle &pass)
        : star::common::IServiceCommandWithReply<get_pass_info::GatheredPassInfo>(), pass(&pass) {};

    const Handle *pass{nullptr};
};
} // namespace star::command_order