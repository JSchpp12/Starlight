#include "starlight/event/RenderReadyForFinalization.hpp"

#include <star_common/HandleTypeRegistry.hpp>

namespace star::event
{
RenderReadyForFinalization::RenderReadyForFinalization(core::device::StarDevice &device,
                                                       vk::Semaphore finalDoneSemaphore)
    : common::IEvent(common::HandleTypeRegistry::instance().registerType(GetRenderReadyForFinalizationTypeName())),
      m_device(device), m_finalDoneSemaphore(std::move(finalDoneSemaphore))
{
}
} // namespace star::event