#include "SystemContext.hpp"

star::core::SystemContext::SystemContext(RenderingInstance &&renderingInstance)
    : m_instance(std::move(renderingInstance))
{
}

void star::core::SystemContext::createDevice(const uint64_t &frameIndex, const uint8_t &numOfFramesInFlight, std::set<Rendering_Features> requiredFeatures,
                                             StarWindow &window)
{
    m_deviceInfos.emplace_back(devices::DeviceContext(numOfFramesInFlight, m_instance, requiredFeatures, window));
}
