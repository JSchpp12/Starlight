#pragma once

#include <star_common/IEvent.hpp>

#include <string_view>

namespace star::event
{
namespace descriptor_pool_ready
{
constexpr const char *GetUniqueTypeName()
{
    return "EvtDPR";
}
} // namespace descriptor_pool_ready
class DescriptorPoolReady : public common::IEvent
{
  public:
    static constexpr std::string_view GetUniqueTypeName()
    {
        return descriptor_pool_ready::GetUniqueTypeName();
    }

    DescriptorPoolReady();
};
}