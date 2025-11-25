#pragma once

#include "LinearHandleContainer.hpp"
#include "RenderingInstance.hpp"
#include "RenderingSurface.hpp"
#include "StarWindow.hpp"
#include "core/device/DeviceContext.hpp"

#include <starlight/common/HandleTypeRegistry.hpp>

#include <vulkan/vulkan.hpp>

namespace star::core
{
class SystemContext
{
  public:
    SystemContext(RenderingInstance &&renderingInstance);

    void createDevice(const Handle &deviceID, const uint64_t &frameIndex, const uint8_t &numOfFramesInFlight,
                      std::set<star::Rendering_Features> requiredFeatures, StarWindow &window,
                      const std::set<Rendering_Device_Features> &requiredRenderingDeviceFeatures);

    device::DeviceContext &getContext(const Handle &handle)
    {
        return m_contexts.get(handle);
    }
    RenderingInstance &getRenderingInstance()
    {
        return m_instance;
    }

    LinearHandleContainer<device::DeviceContext, 1> &getAllDevices()
    {
        return m_contexts;
    }

  private:
    RenderingInstance m_instance;
    LinearHandleContainer<device::DeviceContext, 1> m_contexts =
        LinearHandleContainer<device::DeviceContext, 1>(common::special_types::DeviceTypeName);
};
} // namespace star::core