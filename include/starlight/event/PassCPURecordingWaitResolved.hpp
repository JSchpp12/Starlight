#pragma once

#include <starlight/core/transfer/ReadinessFailurePolicy.hpp>

#include <star_common/Handle.hpp>
#include <star_common/IEvent.hpp>

#include <boost/container/small_vector.hpp>

#include <string_view>
#include <utility>
#include <vector>

namespace star::event
{
namespace pass_cpu_recording_wait_resolved
{
inline constexpr const char *GetUniqueTypeName()
{
    return "EvtPCWR";
}
} // namespace pass_cpu_recording_wait_resolved

/// @brief CPU recording wait outcome for one command buffer.
/// @details `failedResources` is terminal; `pendingResources` means the pass skipped this frame and will retry. Both
/// empty means the pass recorded normally.
struct PassWaitResultInfo
{
    Handle pass{};
    std::vector<Handle> failedResources;
    std::vector<Handle> pendingResources;
    ReadinessFailurePolicy onFailure{ReadinessFailurePolicy::BlockUntilReady};
};

/// @brief All command buffers' CPU recording wait outcomes for one frame. Emitted once on the main thread after
/// submission.
class PassCPURecordingWaitResolved : public common::IEvent
{
  public:
    using Results = boost::container::small_vector<PassWaitResultInfo, 5>;

    explicit PassCPURecordingWaitResolved(Results results);

    static constexpr std::string_view GetUniqueTypeName()
    {
        return pass_cpu_recording_wait_resolved::GetUniqueTypeName();
    }

    [[nodiscard]] const Results &getResults() const noexcept
    {
        return m_results;
    }

  private:
    Results m_results;
};
} // namespace star::event
