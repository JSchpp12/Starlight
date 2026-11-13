#pragma once

#include <starlight/core/transfer/ReadinessCondition.hpp>
#include <starlight/core/transfer/ReadinessFailurePolicy.hpp>

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

/// @brief Require a command buffer to block until transfer resources are CPU-published before it records.
/// @details Resources may be Image-manager handles, Buffer-manager handles, or legacy `"stBuffer"`
/// ManagerRenderResource handles (engine-uploaded vertex/instance/uniform buffers). An initialized `pass` targets that
/// pass's container; leaving it unset targets the frame-wide container drained immediately before the finalization
/// phase records.
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

    ForceCPURecordingWait &addCondition(ReadinessCondition condition)
    {
        conditions.push_back(std::move(condition));
        return *this;
    }

    ForceCPURecordingWait &setFailurePolicy(ReadinessFailurePolicy value)
    {
        onFailure = value;
        return *this;
    }

    Handle pass{};
    /// @brief Resources whose readiness kind is inferred from the handle type.
    std::vector<Handle> resources{};
    /// @brief Explicitly typed readiness conditions (take precedence over inferred ones for the same resource).
    std::vector<ReadinessCondition> conditions{};
    ReadinessFailurePolicy onFailure{ReadinessFailurePolicy::BlockUntilReady};
};
} // namespace star::command_order
