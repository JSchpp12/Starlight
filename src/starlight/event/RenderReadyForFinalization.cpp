#include "starlight/event/RenderReadyForFinalization.hpp"

#include <star_common/HandleTypeRegistry.hpp>

namespace star::event
{
RenderReadyForFinalization::RenderReadyForFinalization(core::device::StarDevice &device)
    : common::IEvent(common::HandleTypeRegistry::instance().registerType(GetRenderReadyForFinalizationTypeName)),
      m_device(device)
{
}
} // namespace star::event