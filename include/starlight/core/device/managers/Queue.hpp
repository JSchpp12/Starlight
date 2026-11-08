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
        (void)device;
        queue.getVulkanQueue() = VK_NULL_HANDLE;
    }
};

constexpr std::string_view GetQueueEventTypeName = "stQueE";

/// @brief Compile-time name of the queue resource type, used for handle registration and failure logging.
struct QueueResourceTypeName
{
    static constexpr std::string_view GetUniqueTypeName()
    {
        return common::special_types::QueueTypeName;
    }
};

class Queue : public Manager<QueueRecord, QueueRequest, 32, QueueResourceTypeName>
{
  public:
    Queue() : Manager<QueueRecord, QueueRequest, 32, QueueResourceTypeName>()
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