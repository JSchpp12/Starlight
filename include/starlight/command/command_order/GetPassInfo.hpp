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
    GatheredPassInfo() = default;
    GatheredPassInfo(bool isTriggeredThisFrame, const uint32_t *queueFamilyIndex,
                     const std::vector<bool> *wasProcessedOnLastFrame,
                     const std::vector<star::service::command_order::EdgeDescription> *edges)
        : isTriggeredThisFrame(isTriggeredThisFrame), queueFamilyIndex(queueFamilyIndex),
          wasProcessedOnLastFrame(wasProcessedOnLastFrame), edges(edges)
    {
    }
    
    bool isTriggeredThisFrame = false;
    const uint32_t *queueFamilyIndex = nullptr;
    const std::vector<bool> *wasProcessedOnLastFrame = nullptr;
    const std::vector<star::service::command_order::EdgeDescription> *edges = nullptr;
};
} // namespace get_pass_info

struct GetPassInfo : public star::common::IServiceCommandWithReply<get_pass_info::GatheredPassInfo>
{
    GetPassInfo(uint16_t type, Handle pass)
        : star::common::IServiceCommandWithReply<get_pass_info::GatheredPassInfo>(std::move(type)),
          pass(std::move(pass)){};
    explicit GetPassInfo(Handle pass)
        : star::common::IServiceCommandWithReply<get_pass_info::GatheredPassInfo>(), pass(std::move(pass)){};

    Handle pass;
};
} // namespace star::command_order