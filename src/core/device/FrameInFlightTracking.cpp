#include "core/device/FrameInFlightTracking.hpp"

#include <cassert>

namespace star::core
{
uint64_t &FrameInFlightTracking::getNumOfTimesFrameProcessed(const uint8_t &frameInFlightIndex)
{
    assert(frameInFlightIndex < m_numOfTimesFrameProcessed.size());

    return m_numOfTimesFrameProcessed[frameInFlightIndex];
}

const uint64_t &FrameInFlightTracking::getNumOfTimesFrameProcessed(const uint8_t &frameInFlightIndex) const
{
    assert(frameInFlightIndex < m_numOfTimesFrameProcessed.size());

    return m_numOfTimesFrameProcessed[frameInFlightIndex];
}

} // namespace star::core