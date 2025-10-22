#include "SystemContext.hpp"

star::core::SystemContext::SystemContext(RenderingInstance &&renderingInstance)
    : m_instance(std::move(renderingInstance))
{
}

void star::core::SystemContext::createDevice(const Handle &deviceID, const uint64_t &frameIndex,
                                             const uint8_t &numOfFramesInFlight,
                                             std::set<Rendering_Features> requiredFeatures, StarWindow &window,
                                             const std::set<Rendering_Device_Features> &requiredRenderingDeviceFeatures)
{
    auto handle = m_contexts.insert(device::DeviceContext());
    m_contexts.get(handle).init(handle, numOfFramesInFlight, m_instance, requiredFeatures, window, requiredRenderingDeviceFeatures);
}
