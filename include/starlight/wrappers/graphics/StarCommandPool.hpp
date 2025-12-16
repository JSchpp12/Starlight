#pragma once

#include <vulkan/vulkan.hpp>

namespace star
{
class StarCommandPool
{
  public:
  StarCommandPool(vk::Device vulkanDevice, const uint32_t &familyIndex, const bool &setAutoReset); 
  ~StarCommandPool(); 

  StarCommandPool(const StarCommandPool&) = delete;
  StarCommandPool& operator=(const StarCommandPool&) = delete;

  vk::CommandPool &getVulkanCommandPool(){return this->commandPool;}
  private:
  vk::Device vulkanDevice = vk::Device();
  vk::CommandPool commandPool = vk::CommandPool();

  static vk::CommandPool CreateCommandPool(vk::Device &device, const uint32_t &familyIndex, const bool &setAutoReset); 
};
} // namespace star