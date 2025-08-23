#pragma once

#include "DeviceContext.hpp"
#include "RenderingInstance.hpp"
#include "RenderingSurface.hpp"
#include "StarWindow.hpp"

#include <vulkan/vulkan.hpp>

namespace star::core
{
class DeviceManager
{
  public:
    DeviceManager(RenderingInstance &&renderingInstance);

    void createDevice(const uint64_t &frameIndex, const uint8_t &numOfFramesInFlight, std::set<star::Rendering_Features> requiredFeatures,
                      StarWindow &window);

    DeviceContext &getContext()
    {
        assert(m_deviceInfos.size() > 0 && "No devices have been added to the manager yet");

        return m_deviceInfos.front();
    }
    RenderingInstance &getRenderingInstance()
    {
        return m_instance;
    }

  private:
    RenderingInstance m_instance;
    std::vector<DeviceContext> m_deviceInfos = std::vector<DeviceContext>();
};
} // namespace star::core