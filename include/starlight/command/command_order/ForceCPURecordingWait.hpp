#pragma once

#include <star_common/Handle.hpp>
#include <star_common/IServiceCommand.hpp>

#include <string_view>
#include <utility>
#include <vector>

namespace star::command_order
{
namespace force_cpu_recording_wait
{
inline constexpr const char *GetTypeName()
{
    return "coFCW";
}
} // namespace force_cpu_recording_wait

/// @brief Require the owning pass to block until transfer resources are CPU-published before it records.
/// @details The pass must be a declared command buffer. Resources may be Image-manager handles, Buffer-manager
/// handles, or legacy `"stBuffer"` ManagerRenderResource handles (engine-uploaded vertex/instance/uniform buffers).
/// The waiter blocks on each resource's publication/completion signal before the pass records.
struct ForceCPURecordingWait : public star::common::IServiceCommand
{
    static constexpr std::string_view GetUniqueTypeName()
    {
        return force_cpu_recording_wait::GetTypeName();
    }

    ForceCPURecordingWait() = default;

    explicit ForceCPURecordingWait(Handle pass) : pass(std::move(pass))
    {
    }

    ForceCPURecordingWait &setPass(Handle value)
    {
        pass = std::move(value);
        return *this;
    }

    ForceCPURecordingWait &addResource(Handle value)
    {
        resources.push_back(std::move(value));
        return *this;
    }

    Handle pass{};
    std::vector<Handle> resources{};
};
} // namespace star::command_order
