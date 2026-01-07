#include "starlight/core/device/managers/Queue.hpp"

namespace star::core::device::manager
{
QueueRecord Queue::createRecord(QueueRequest &&request) const
{
    return {.queue = {request.queue, request.parentQueueFamilyIndex}};
}
} // namespace star::core::device::manager