#include "StarCommandPool.hpp"

star::StarCommandPool::StarCommandPool(vk::Device vulkanDevice, const uint32_t &familyIndex, const bool &setAutoReset)
: vulkanDevice(vulkanDevice), commandPool(CreateCommandPool(vulkanDevice, familyIndex, setAutoReset)){

}

star::StarCommandPool::~StarCommandPool(){
    this->vulkanDevice.destroyCommandPool(this->commandPool); 
}

vk::CommandPool star::StarCommandPool::CreateCommandPool(vk::Device &vulkanDevice, const uint32_t &familyIndex, const bool &setAutoReset){
    vk::CommandPoolCreateInfo commandPoolInfo{};
    commandPoolInfo.sType = vk::StructureType::eCommandPoolCreateInfo;
    commandPoolInfo.queueFamilyIndex = familyIndex;
    
    if (setAutoReset)
        commandPoolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

    vk::CommandPool pool = vulkanDevice.createCommandPool(commandPoolInfo);

    if (!pool)
    {
        throw std::runtime_error("unable to create pool");
    }

    return pool;
}