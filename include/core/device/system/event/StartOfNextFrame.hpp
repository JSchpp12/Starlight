#pragma once

#include <starlight/common/HandleTypeRegistry.hpp>
#include <starlight/common/IEvent.hpp>

#include <stdint.h>

namespace star::core::device::system::event
{
constexpr std::string StartOfNextFrameName()
{
    return "star::event::start_of_next_frame";
}
class StartOfNextFrame : public star::common::IEvent
{
  public:
    StartOfNextFrame(const uint64_t &frameIndex, const uint8_t &frameInFlightIndex)
        : common::IEvent(common::HandleTypeRegistry::instance().registerType(StartOfNextFrameName())),
          m_frameIndex(frameIndex), m_frameInFlightIndex(frameInFlightIndex)
    {
    }

    const uint64_t &getFrameIndex() const
    {
        return m_frameIndex;
    }

    const uint8_t &getFrameInFlightIndex() const
    {
        return m_frameInFlightIndex;
    }

  private:
    const uint64_t &m_frameIndex;
    const uint8_t &m_frameInFlightIndex;
};
} // namespace star::core::device::system::event