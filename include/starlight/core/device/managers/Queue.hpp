#pragma once

#include "core/device/managers/ManagerEventBusTies.hpp"
#include "starlight/wrappers/graphics/StarQueue.hpp"

#include <string_view>

namespace star::core::device::manager
{

struct QueueRequest
{
    vk::Queue queue; 
    uint32_t parentQueueFamilyIndex;
};

struct QueueRecord
{
    star::StarQueue queue;

    bool isReady() const
    {
        return queue.getVulkanQueue() != VK_NULL_HANDLE;
    }

    void cleanupRender(device::StarDevice &device)
    {
    }
};

constexpr std::string_view GetQueueEventTypeName = "star::core::device::manager::queue";

class Queue : public Manager<QueueRecord, QueueRequest, 36>
{
  public:
    Queue()
        : Manager<QueueRecord, QueueRequest, 36>(common::special_types::QueueTypeName)
    {
    }
    virtual ~Queue() = default;

  protected:
    virtual QueueRecord createRecord(QueueRequest &&request) const;
};
} // namespace star::core::device::manager