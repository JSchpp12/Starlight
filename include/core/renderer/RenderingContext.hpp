#pragma once

#include "MappedHandleContainer.hpp"
#include "StarPipeline.hpp"
#include "core/device/DeviceContext.hpp"

#include <vector>

namespace star::core::renderer
{
/// @brief Contains information needed for objects to process their rendering commands
class RenderingContext
{
  public:
    vk::Extent2D targetResolution;
    StarPipeline *pipeline = nullptr;
    MappedHandleContainer<vk::Buffer, star::Handle_Type::buffer> bufferTransferRecords =
        MappedHandleContainer<vk::Buffer, star::Handle_Type::buffer>();
    MappedHandleContainer<vk::Semaphore, star::Handle_Type::semaphore> recordDependentSemaphores =
        MappedHandleContainer<vk::Semaphore, star::Handle_Type::semaphore>();
    MappedHandleContainer<vk::Fence, star::Handle_Type::fence> recordDependentFence =
        MappedHandleContainer<vk::Fence, star::Handle_Type::fence>();

    void addBufferToRenderingContext(core::device::DeviceContext &context, const Handle &handle)
    {
        assert(handle.getType() == star::Handle_Type::buffer);

        bufferTransferRecords.manualInsert(
            handle, context.getManagerRenderResource().getBuffer(context.getDeviceID(), handle).getVulkanBuffer());
    }
};
} // namespace star::core::renderer