#include "DeviceManager.hpp"

star::core::DeviceManager::DeviceManager(RenderingInstance &&renderingInstance) : m_instance(std::move(renderingInstance))
{

}

void star::core::DeviceManager::addDevice(StarDevice&& device)
{
    m_deviceInfos.emplace_back(std::move(device)); 
}
