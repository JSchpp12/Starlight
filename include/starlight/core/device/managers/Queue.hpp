#pragma once

#include "core/device/managers/Manager.hpp"
#include "starlight/wrappers/graphics/StarQueue.hpp"

#include <string_view>

namespace star::core::device::manager
{

struct QueueRequest
{
    StarQueue queue;
};

struct QueueRecord
{
    star::StarQueue queue;

    bool isReady() const
    {
        bool notReady = !queue;
        return !notReady;
    }

    void cleanupRender(device::StarDevice &device)
    {
        queue.getVulkanQueue() = VK_NULL_HANDLE;
    }
};

constexpr std::string_view GetQueueEventTypeName = "star::core::device::manager::queue";

class Queue : public Manager<QueueRecord, QueueRequest, 32>
{
  public:
    Queue() : Manager<QueueRecord, QueueRequest, 32>(common::special_types::QueueTypeName)
    {
    }
    Queue(const Queue &) = delete;
    Queue &operator=(const Queue &) = delete;
    Queue(Queue &&other) = default;
    Queue &operator=(Queue &&other) = default;
    virtual ~Queue() = default;

  protected:
    virtual QueueRecord createRecord(QueueRequest &&request) const;
};
} // namespace star::core::device::manager