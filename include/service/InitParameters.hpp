#pragma once

#include "ManagerRenderResource.hpp"
#include "RenderingSurface.hpp"
#include "SwapChainSupportDetails.hpp"
#include "TaskManager.hpp"
#include "core/device/FrameInFlightTracking.hpp"
#include "device/StarDevice.hpp"
#include "device/managers/GraphicsContainer.hpp"
#include "device/managers/ManagerCommandBuffer.hpp"
#include "device/managers/Pipeline.hpp"
#include "device/system/EventBus.hpp"
#include "tasks/TaskFactory.hpp"

namespace star::service
{
struct InitParameters
{
    Handle &deviceID;
    core::RenderingSurface &surface; 
    core::device::StarDevice &device;
    core::device::system::EventBus &eventBus;
    job::TaskManager &taskManager;
    core::device::manager::GraphicsContainer &graphicsManagers;
    core::device::manager::ManagerCommandBuffer &commandBufferManager;
    job::TransferWorker &transferWorker;
    ManagerRenderResource &renderResourceManager;
};
} // namespace star::service