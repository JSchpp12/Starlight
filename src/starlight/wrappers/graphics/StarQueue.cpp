#include "StarQueue.hpp"

star::StarQueue::StarQueue(vk::Queue queue, const uint32_t &parentQueueFamilyIndex)
    : queue(queue), parentQueueFamilyIndex(parentQueueFamilyIndex)
{
}