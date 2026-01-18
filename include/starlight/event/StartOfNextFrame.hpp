#pragma once

#include <star_common/FrameTracker.hpp>
#include <star_common/IEvent.hpp>

#include <stdint.h>
#include <string_view>

namespace star::event
{
constexpr std::string_view GetStartOfNextFrameTypeName = "star::event::start_of_next_frame";

class StartOfNextFrame : public star::common::IEvent
{
  public:
    StartOfNextFrame(const common::FrameTracker &frameTracker);

    const common::FrameTracker &getFrameTracker() const
    {
        return m_frameTracker;
    }

  private:
    const common::FrameTracker &m_frameTracker;
};
} // namespace star::event