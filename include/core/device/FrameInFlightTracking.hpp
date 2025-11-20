#pragma once

#include <cstdint>
#include <vector>

namespace star::core
{
class FrameInFlightTracking
{
  public:
    FrameInFlightTracking() = default;
    FrameInFlightTracking(const uint8_t &numFramesInFlight)
        : m_numOfTimesFrameProcessed(std::vector<uint64_t>(numFramesInFlight, uint64_t(0)))
    {
    }
    // frame counter for this frame in flight index. Global counter might be 4 but this counter would be 1 under 3
    // frames in flight. As this frame in flight has been processed 1 time.

    uint64_t &getNumOfTimesFrameProcessed(const uint8_t &frameInFlightIndex);
    const uint64_t &getNumOfTimesFrameProcessed(const uint8_t &frameInFlightIndex) const;

  private:
    std::vector<uint64_t> m_numOfTimesFrameProcessed;
};
} // namespace star::core