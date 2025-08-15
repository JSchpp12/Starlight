#include "DeviceManager.hpp"

star::core::DeviceManager::DeviceManager(RenderingInstance &&renderingInstance) : m_instance(std::move(renderingInstance))
{

}

void star::core::DeviceManager::createDevice(std::set<Rendering_Features> requiredFeatures, StarWindow &window)
{
    m_deviceInfos.push_back(DeviceContext(m_instance, requiredFeatures, window)); 
}
