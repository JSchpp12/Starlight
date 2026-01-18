#pragma once

#include "core/MappedHandleContainer.hpp"
#include "core/device/StarDevice.hpp"
#include "core/device/managers/ManagerCommandBuffer.hpp"
#include "core/device/managers/Semaphore.hpp"
#include "core/device/managers/Queue.hpp"
#include "job/TaskManager.hpp"

#include <star_common/EventBus.hpp>
#include <star_common/FrameTracker.hpp>

namespace star::service::detail::screen_capture
{
struct DeviceInfo
{
    core::device::StarDevice *device = nullptr;
    core::device::manager::ManagerCommandBuffer *commandManager = nullptr;
    star::common::EventBus *eventBus = nullptr;
    core::device::manager::Semaphore *semaphoreManager = nullptr;
    core::device::manager::Queue *queueManager = nullptr; 
    job::TaskManager *taskManager = nullptr;
    star::common::FrameTracker *flightTracker = nullptr;
};
} // namespace star::service::detail::screen_capture
