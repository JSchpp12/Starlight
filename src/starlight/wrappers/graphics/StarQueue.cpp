#include "StarQueue.hpp"

namespace star
{
StarQueue::StarQueue(uint32_t parentQueueFamilyIndex, vk::Queue queue, vk::QueueFlags caps, bool supportsPresentation)
    : m_parentQueueFamilyIndex(std::move(parentQueueFamilyIndex)), m_queue(queue), m_caps(std::move(caps)),
      m_supportsPresentation(supportsPresentation)
{
}

bool StarQueue::isCompatibleWith(const vk::QueueFlags &requestedCaps) const
{
    return ((m_caps & requestedCaps) == requestedCaps);
}
} // namespace star
