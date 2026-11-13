#pragma once

#include <cstdint>

namespace star
{
/// @brief What a consumer should do when a readiness condition fails terminally.
/// @details `BlockUntilReady` is the default: a pending condition is retried, a terminal failure is reported to the
/// pass owner (which applies the strictest policy among the failing conditions).
enum class ReadinessFailurePolicy : std::uint8_t
{
    BlockUntilReady = 0,
    SkipPass,
    KeepPrevious,
    Terminate
};

/// @brief Ordering used when a pass has several failing conditions with different policies.
[[nodiscard]] constexpr int ReadinessFailurePolicyPriority(const ReadinessFailurePolicy policy) noexcept
{
    switch (policy)
    {
    case ReadinessFailurePolicy::Terminate:
        return 3;
    case ReadinessFailurePolicy::KeepPrevious:
        return 2;
    case ReadinessFailurePolicy::SkipPass:
        return 1;
    case ReadinessFailurePolicy::BlockUntilReady:
    default:
        return 0;
    }
}
} // namespace star
