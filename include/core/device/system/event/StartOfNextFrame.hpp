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
    StartOfNextFrame(const uint64_t &frameIndex)
        : common::IEvent(common::HandleTypeRegistry::instance().registerType(StartOfNextFrameName())),
          m_frameIndex(frameIndex)
    {
    }

    const uint64_t &getFrameIndex() const
    {
        return m_frameIndex;
    }

  private:
    const uint64_t &m_frameIndex;
};
} // namespace star::core::device::system::event