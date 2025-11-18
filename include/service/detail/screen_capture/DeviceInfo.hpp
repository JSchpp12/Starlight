#pragma once

#include "core/MappedHandleContainer.hpp"
#include "core/RenderingSurface.hpp"
#include "core/device/StarDevice.hpp"
#include "core/device/managers/ManagerCommandBuffer.hpp"
#include "core/device/managers/Semaphore.hpp"
#include "core/device/system/EventBus.hpp"
#include "job/TaskManager.hpp"

namespace star::service::detail::screen_capture
{
struct DeviceInfo
{
    core::device::StarDevice *device = nullptr;
    core::device::manager::ManagerCommandBuffer *commandManager = nullptr;
    core::RenderingSurface *surface = nullptr;
    core::device::system::EventBus *eventBus = nullptr;
    core::device::manager::Semaphore *semaphoreManager = nullptr;
    job::TaskManager *taskManager = nullptr;
    const uint64_t *currentFrameCounter = nullptr;
    uint8_t numFramesInFlight;
};
} // namespace star::service::detail::screen_capture
