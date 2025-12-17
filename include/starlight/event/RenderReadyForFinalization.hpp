#pragma once

#include <starlight/core/device/StarDevice.hpp>

#include <star_common/IEvent.hpp>

namespace star::event
{

constexpr std::string_view GetRenderReadyForFinalizationTypeName = "star::event::finalization";
class RenderReadyForFinalization : public common::IEvent
{
  public:
    explicit RenderReadyForFinalization(core::device::StarDevice &device);

    virtual ~RenderReadyForFinalization() = default;

    core::device::StarDevice &getDevice() const
    {
        return m_device;
    }
  private:
    core::device::StarDevice &m_device;
};
} // namespace star::event