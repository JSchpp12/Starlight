#include "SystemContext.hpp"

star::core::SystemContext::SystemContext(RenderingInstance &&renderingInstance)
    : m_instance(std::move(renderingInstance))
{
}

void star::core::SystemContext::createDevice(const device::DeviceID &deviceID, const uint64_t &frameIndex, const uint8_t &numOfFramesInFlight,
                                             std::set<Rendering_Features> requiredFeatures, StarWindow &window, const std::set<Rendering_Device_Features> &requiredRenderingDeviceFeatures)
{
    m_deviceInfos.emplace_back(
        device::DeviceContext(numOfFramesInFlight, deviceID, m_instance, requiredFeatures, window, requiredRenderingDeviceFeatures));
}
