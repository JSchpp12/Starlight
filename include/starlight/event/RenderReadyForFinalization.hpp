#pragma once

#include <starlight/core/device/StarDevice.hpp>

#include <star_common/IEvent.hpp>

namespace star::event
{
inline constexpr const char *GetRenderReadyForFinalizationTypeName()
{
    return "eFinal";
}

class RenderReadyForFinalization : public common::IEvent
{
  public:
    RenderReadyForFinalization(core::device::StarDevice &device, vk::Semaphore finalDoneSemaphore);

    virtual ~RenderReadyForFinalization() = default;

    core::device::StarDevice &getDevice() const
    {
        return m_device;
    }
    vk::Semaphore &getFinalDoneSemaphore()
    {
        return m_finalDoneSemaphore;
    }
    const vk::Semaphore &getFinalDoneSemaphore() const
    {
        return m_finalDoneSemaphore;
    }

  private:
    core::device::StarDevice &m_device;
    vk::Semaphore m_finalDoneSemaphore;
};
} // namespace star::event