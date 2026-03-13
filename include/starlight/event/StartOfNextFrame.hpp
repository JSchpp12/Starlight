#pragma once

#include <star_common/FrameTracker.hpp>
#include <star_common/IEvent.hpp>

namespace star::event
{
namespace start_of_next_frame
{
inline constexpr const char *GetUniqueTypeName()
{
    return "EvtSONF";
}
} // namespace start_of_next_frame

class StartOfNextFrame : public star::common::IEvent
{
  public:
    static constexpr std::string_view GetUniqueTypeName()
    {
        return start_of_next_frame::GetUniqueTypeName();
    }
    StartOfNextFrame(const common::FrameTracker &frameTracker);

    const common::FrameTracker &getFrameTracker() const
    {
        return m_frameTracker;
    }

  private:
    const common::FrameTracker &m_frameTracker;
};
} // namespace star::event