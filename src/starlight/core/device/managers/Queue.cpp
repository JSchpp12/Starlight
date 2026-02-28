#include "starlight/core/device/managers/Queue.hpp"

namespace star::core::device::manager
{
QueueRecord Queue::createRecord(QueueRequest &&request) const
{
    return {.queue = std::move(request.queue)};
}
} // namespace star::core::device::manager