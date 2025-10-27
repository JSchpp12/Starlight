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

    void addBufferToRenderingContext(core::device::DeviceContext &context, const Handle &handle)
    {
        assert(handle.getType() == star::Handle_Type::buffer);

        context.getManagerRenderResource().waitForReady(context.getDeviceID(), handle);

        bufferTransferRecords.manualInsert(
            handle, context.getManagerRenderResource().getBuffer(context.getDeviceID(), handle).getVulkanBuffer());
    }
};
} // namespace star::core::renderer