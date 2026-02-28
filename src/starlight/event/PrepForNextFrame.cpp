#include "starlight/event/PrepForNextFrame.hpp"

#include <star_common/HandleTypeRegistry.hpp>

namespace star::event
{
PrepForNextFrame::PrepForNextFrame(common::FrameTracker &frameTracker)
    : common::IEvent(star::common::HandleTypeRegistry::instance().registerType(GetPrepForNextFrameEventTypeName())),
      m_frameTracker(&frameTracker)
{
}

} // namespace star::event