#include "device/managers/Semaphore.hpp"

namespace star::core::device::manager{
vk::Semaphore Semaphore::CreateSemaphore(device::StarDevice &device, const bool &isTimelineSemaphore){
    auto typeInfo = vk::SemaphoreTypeCreateInfo()
        .setSemaphoreType(isTimelineSemaphore ? vk::SemaphoreType::eTimeline : vk::SemaphoreType::eBinary)
        .setInitialValue(0);

    auto createInfo = vk::SemaphoreCreateInfo()
        .setPNext(&typeInfo); 

    vk::Semaphore newSemaphore = device.getVulkanDevice().createSemaphore(createInfo);
    
    if (newSemaphore == VK_NULL_HANDLE){
        throw std::runtime_error("Failed to create semaphore"); 
    }

    return newSemaphore; 
}
}