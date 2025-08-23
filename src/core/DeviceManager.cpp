#include "DeviceManager.hpp"

star::core::DeviceManager::DeviceManager(RenderingInstance &&renderingInstance)
    : m_instance(std::move(renderingInstance))
{
}

void star::core::DeviceManager::createDevice(const uint64_t &frameIndex, const uint8_t &numOfFramesInFlight, std::set<Rendering_Features> requiredFeatures,
                                             StarWindow &window)
{
    m_deviceInfos.emplace_back(DeviceContext(numOfFramesInFlight, m_instance, requiredFeatures, window));
}
