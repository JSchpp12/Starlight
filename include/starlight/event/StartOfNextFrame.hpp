#pragma once

#include <star_common/FrameTracker.hpp>
#include <star_common/IEvent.hpp>

namespace star::event
{
inline constexpr const char *GetStartOfNextFrameTypeName()
{
    return "eStFrame";
}

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