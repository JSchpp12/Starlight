#pragma once

#include <vulkan/vulkan.hpp>

namespace star
{
class StarCommandPool
{
  public:
    StarCommandPool() = default;
    StarCommandPool(vk::Device vulkanDevice, const uint32_t &familyIndex, const bool &setAutoReset);

    void cleanupRender(vk::Device &device);

    vk::CommandPool getVulkanCommandPool() const
    {
        return commandPool;
    }

  private:
    vk::CommandPool commandPool = VK_NULL_HANDLE;

    static vk::CommandPool CreateCommandPool(vk::Device &device, const uint32_t &familyIndex, const bool &setAutoReset);
};
} // namespace star