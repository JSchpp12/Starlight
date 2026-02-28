#pragma once

#include "ManagerRenderResource.hpp"
#include "SwapChainSupportDetails.hpp"
#include "TaskManager.hpp"
#include "device/StarDevice.hpp"
#include "device/managers/GraphicsContainer.hpp"
#include "device/managers/ManagerCommandBuffer.hpp"
#include "device/managers/Pipeline.hpp"
#include "starlight/core/CommandBus.hpp"
#include "starlight/core/CommandSubmitter.hpp"

#include <star_common/EventBus.hpp>
#include <star_common/FrameTracker.hpp>

namespace star::service
{
struct InitParameters
{
    core::CommandSubmitter cmdSubmitter;
    Handle &deviceID;
    core::device::StarDevice &device;
    common::EventBus &eventBus;
    core::CommandBus &commandBus;
    job::TaskManager &taskManager;
    core::device::manager::GraphicsContainer &graphicsManagers;
    core::device::manager::ManagerCommandBuffer &commandBufferManager;
    ManagerRenderResource &renderResourceManager;
    common::FrameTracker &flightTracker;
};
} // namespace star::service