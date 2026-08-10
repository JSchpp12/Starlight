#pragma once

#include "LinearHandleContainer.hpp"
#include "RenderingInstance.hpp"
#include "core/device/DeviceContext.hpp"

#include <star_common/HandleTypeRegistry.hpp>

#include <vulkan/vulkan.hpp>

namespace star::core
{
class SystemContext
{
  public:
    explicit SystemContext(RenderingInstance *renderingInstance);

    Handle registerDevice(core::device::DeviceContext context);

    void prepareForNextFrame(const uint8_t &mainRendererFrameInFlightIndex);

    device::DeviceContext &getContext(const Handle &handle)
    {
        return m_contexts.get(handle);
    }

    LinearHandleContainer<device::DeviceContext> &getAllDevices()
    {
        return m_contexts;
    }

  private:
    RenderingInstance *m_instance = nullptr;
    LinearHandleContainer<device::DeviceContext> m_contexts =
        LinearHandleContainer<device::DeviceContext>(common::special_types::DeviceTypeName, 1);
};
} // namespace star::core