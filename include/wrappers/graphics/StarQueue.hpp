#pragma once

#include <vulkan/vulkan.hpp>

namespace star
{
class StarQueue
{
  public:
    StarQueue() = default;
    StarQueue(vk::Queue queue, const uint32_t &parentQueueFamilyIndex);

    uint32_t getParentQueueFamilyIndex()
    {
        return this->parentQueueFamilyIndex;
    }

    vk::Queue &getVulkanQueue()
    {
        return this->queue;
    }

  private:
    uint32_t parentQueueFamilyIndex = 0;
    vk::Queue queue = vk::Queue();
};
} // namespace star