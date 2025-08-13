#pragma once

#include "DeviceContext.hpp"
#include "RenderingInstance.hpp"

#include <vulkan/vulkan.hpp>

namespace star::core
{
class DeviceManager
{
  public:
    DeviceManager(RenderingInstance &&renderingInstance);

    void addDevice(StarDevice &&device);
    DeviceContext &getContext()
    {
        assert(m_deviceInfos.size() > 0 && "No devices have been added to the manager yet");

        return m_deviceInfos.front();
    }

  private:
    std::vector<DeviceContext> m_deviceInfos = std::vector<DeviceContext>();
    RenderingInstance m_instance;
};
} // namespace star::core