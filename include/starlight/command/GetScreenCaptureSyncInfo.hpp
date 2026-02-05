#pragma once

#include <star_common/IServiceCommandWithReply.hpp>
#include <star_common/ServiceReply.hpp>

#include <vulkan/vulkan.hpp>

#include <string_view>

namespace star::command
{
namespace get_sync_info
{
inline constexpr std::string_view GetSyncInfoCommandTypeName = "star:getSyncInfo";

struct SyncInfo
{
    const Handle *cmdBuffer = nullptr;
};
} // namespace get_sync_info

struct GetScreenCaptureSyncInfo : public common::IServiceCommandWithReply<get_sync_info::SyncInfo>
{
    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return get_sync_info::GetSyncInfoCommandTypeName;
    }
};

} // namespace star::command