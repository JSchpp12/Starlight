#include "event/StartOfNextFrame.hpp"

#include <star_common/HandleTypeRegistry.hpp>

namespace star::event
{
StartOfNextFrame::StartOfNextFrame(const common::FrameTracker &frameTracker)
    : common::IEvent(common::HandleTypeRegistry::instance().registerType(GetStartOfNextFrameTypeName())),
      m_frameTracker(frameTracker)
{
}

} // namespace star::event