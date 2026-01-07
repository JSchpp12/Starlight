#include "StarQueue.hpp"

star::StarQueue::StarQueue(vk::Queue queue, uint32_t parentQueueFamilyIndex)
    : m_queue(queue), m_parentQueueFamilyIndex(std::move(parentQueueFamilyIndex))
{
}