#pragma once

#include <vulkan/vulkan.hpp>

namespace star
{
class StarQueue
{
  public:
    StarQueue() = default;
    StarQueue(uint32_t parentQueueFamilyIndex, vk::Queue queue, vk::QueueFlags caps, bool supportsPresentation);
    bool operator!() const
    {
        return m_queue == VK_NULL_HANDLE;
    }

    bool isCompatibleWith(const vk::QueueFlags &requestedCaps) const;
    const uint32_t &getParentQueueFamilyIndex() const
    {
        return m_parentQueueFamilyIndex;
    }

    vk::Queue &getVulkanQueue()
    {
        return m_queue;
    }

    bool getDoesSupportPresentation() const
    {
        return m_supportsPresentation;
    }

  private:
    uint32_t m_parentQueueFamilyIndex = 0;
    vk::Queue m_queue = VK_NULL_HANDLE;
    vk::QueueFlags m_caps;
    bool m_supportsPresentation = false;
};
} // namespace star