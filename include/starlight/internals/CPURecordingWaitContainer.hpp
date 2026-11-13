#pragma once

#include <starlight/core/transfer/ReadinessCondition.hpp>
#include <starlight/core/transfer/ReadinessFailurePolicy.hpp>

#include <star_common/Handle.hpp>

#include <vector>

namespace star
{
/// @brief Owns a pass's one-time host-side readiness conditions.
/// @details Conditions are strongly typed (see ReadinessCondition.hpp). A pass runs them on the recording thread
/// before it records. Pending conditions are retained for a later frame; failed conditions are reported with the
/// strictest policy among them so the caller can skip, roll back, or terminate.
class CPURecordingWaitContainer
{
  public:
    using ImageReadiness = star::ImageReadiness;
    using BufferReadiness = star::BufferReadiness;
    using RenderResourceBufferReadiness = star::RenderResourceBufferReadiness;
    using ShaderReadiness = star::ShaderReadiness;
    using PipelineReadiness = star::PipelineReadiness;
    using CPURecordingWait = star::ReadinessCondition;

    struct Result
    {
        std::vector<Handle> failed;
        std::vector<Handle> pending;
        ReadinessFailurePolicy policy{ReadinessFailurePolicy::BlockUntilReady};
        /// @brief True when waits were registered for this submission (even if all were ready).
        bool hadWaits{false};

        [[nodiscard]] bool isReady() const noexcept
        {
            return failed.empty() && pending.empty();
        }
    };

    /// @brief Register waits that must pass before the pass records.
    void add(std::vector<CPURecordingWait> waits,
             ReadinessFailurePolicy policy = ReadinessFailurePolicy::BlockUntilReady);
    void add(CPURecordingWait wait, ReadinessFailurePolicy policy = ReadinessFailurePolicy::BlockUntilReady);
    /// @brief Evaluate every registered wait.
    [[nodiscard]] Result runAll();

    /// @brief True when waits were registered but `runAll` has not evaluated them for the current submission.
    [[nodiscard]] bool hasUnrun() const noexcept
    {
        return m_hasUnrun;
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return m_waits.empty();
    }

    void clear() noexcept
    {
        m_waits.clear();
        m_hasUnrun = false;
    }

  private:
    struct RegisteredWait
    {
        CPURecordingWait condition;
        ReadinessFailurePolicy policy{ReadinessFailurePolicy::BlockUntilReady};
    };

    void insert(CPURecordingWait &&newWait, ReadinessFailurePolicy policy);

    std::vector<RegisteredWait> m_waits;
    bool m_hasUnrun{false};
};
} // namespace star
