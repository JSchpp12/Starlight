#pragma once

#include "enums/Enums.hpp"
#include "starlight/core/Exceptions.hpp"
#include "starlight/core/device/managers/Queue.hpp"
#include "starlight/event/GetQueue.hpp"
#include "starlight/wrappers/graphics/StarQueue.hpp"

#include <star_common/EventBus.hpp>
#include <star_common/Handle.hpp>

namespace star::core::helper
{
inline static star::StarQueue *GetEngineDefaultQueue(common::EventBus &eventBus,
                                                     core::device::manager::Queue &queueManager,
                                                     const star::Queue_Type type)
{
    Handle defaultQueue;

    {
        auto event =
            event::GetQueue::Builder().setQueueData(defaultQueue).setQueueType(type).getEngineDedicatedQueue().build();
        eventBus.emit(std::move(event));
    }

    if (!defaultQueue.isInitialized())
    {
        return nullptr;
    }

    return &queueManager.get(defaultQueue)->queue;
}
} // namespace star::core::helper