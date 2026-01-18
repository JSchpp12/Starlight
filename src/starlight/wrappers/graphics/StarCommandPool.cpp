#include "StarCommandPool.hpp"

star::StarCommandPool::StarCommandPool(vk::Device vulkanDevice, const uint32_t &familyIndex, const bool &setAutoReset)
: commandPool(CreateCommandPool(vulkanDevice, familyIndex, setAutoReset)){

}

void star::StarCommandPool::cleanupRender(vk::Device &device)
{
    if (commandPool != VK_NULL_HANDLE)
    {
        device.destroyCommandPool(commandPool);
        commandPool = VK_NULL_HANDLE;
    }
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