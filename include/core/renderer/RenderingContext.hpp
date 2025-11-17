#pragma once

#include "MappedHandleContainer.hpp"
#include "StarPipeline.hpp"
#include "core/device/DeviceContext.hpp"

#include <starlight/common/HandleTypeRegistry.hpp>

#include <vector>

namespace star::core::renderer
{
/// @brief Contains information needed for objects to process their rendering commands
class RenderingContext
{
  public:
    vk::Extent2D targetResolution;
    StarPipeline *pipeline = nullptr;
    MappedHandleContainer<vk::Buffer> bufferTransferRecords =
        MappedHandleContainer<vk::Buffer>(common::special_types::BufferTypeName());
    MappedHandleContainer<vk::Semaphore> recordDependentSemaphores =
        MappedHandleContainer<vk::Semaphore>(common::special_types::SemaphoreTypeName());
    MappedHandleContainer<vk::Fence> recordDependentFence =
        MappedHandleContainer<vk::Fence>(common::special_types::FenceTypeName());

    void addBufferToRenderingContext(core::device::DeviceContext &context, const Handle &handle)
    {
        assert(handle.getType() ==
               common::HandleTypeRegistry::instance().getType(common::special_types::BufferTypeName()).value());

        bufferTransferRecords.manualInsert(
            handle, context.getManagerRenderResource().getBuffer(context.getDeviceID(), handle).getVulkanBuffer());
    }
};
} // namespace star::core::renderer