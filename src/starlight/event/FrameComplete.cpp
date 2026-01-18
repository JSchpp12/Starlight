#include "starlight/event/FrameComplete.hpp"

#include <star_common/HandleTypeRegistry.hpp>

namespace star::event
{
FrameComplete::FrameComplete()
    : common::IEvent(common::HandleTypeRegistry::instance().registerType(GetFrameCompleteTypeName))
{
}
}