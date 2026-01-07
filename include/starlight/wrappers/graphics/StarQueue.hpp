#pragma once

#include <vulkan/vulkan.hpp>

namespace star
{
class StarQueue
{
  public:
    StarQueue() = default;
    StarQueue(vk::Queue queue, uint32_t parentQueueFamilyIndex);

    const uint32_t &getParentQueueFamilyIndex()
    {
        return m_parentQueueFamilyIndex;
    }
    const uint32_t &getParentQueueFamilyIndex() const
    {
        return m_parentQueueFamilyIndex;
    }

    vk::Queue &getVulkanQueue()
    {
        return m_queue;
    }
    const vk::Queue &getVulkanQueue() const
    {
        return m_queue;
    }

  private:
    uint32_t m_parentQueueFamilyIndex = 0;
    vk::Queue m_queue = VK_NULL_HANDLE;
};
} // namespace star