#pragma once

#include <cstdint>

namespace star::core
{
struct FrameInFlightTracking
{
    // frame counter for this frame in flight index. Global counter might be 4 but this counter would be 1 under 3
    // frames in flight. As this frame in flight has been processed 1 time.
    uint64_t numOfTimesFrameProcessed = 0;
};
} // namespace star::core