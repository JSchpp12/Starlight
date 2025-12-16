#pragma once

#include "core/MappedHandleContainer.hpp"
#include "core/device/StarDevice.hpp"
#include "core/device/FrameInFlightTracking.hpp"
#include "core/device/managers/ManagerCommandBuffer.hpp"
#include "core/device/managers/Semaphore.hpp"
#include "job/TaskManager.hpp"

#include <star_common/EventBus.hpp>

namespace star::service::detail::screen_capture
{
struct DeviceInfo
{
    core::device::StarDevice *device = nullptr;
    core::device::manager::ManagerCommandBuffer *commandManager = nullptr;
    star::common::EventBus *eventBus = nullptr;
    core::device::manager::Semaphore *semaphoreManager = nullptr;
    const core::FrameInFlightTracking *frameTracker = nullptr;
    job::TaskManager *taskManager = nullptr;
    const uint64_t *currentFrameCounter = nullptr;
    uint8_t numFramesInFlight = 0;
};
} // namespace star::service::detail::screen_capture
