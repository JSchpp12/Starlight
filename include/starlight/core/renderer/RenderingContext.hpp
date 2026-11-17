#pragma once

#include "MappedHandleContainer.hpp"
#include "StarPipeline.hpp"
#include "StarTextures/Texture.hpp"
#include "core/device/DeviceContext.hpp"
#include "starlight/event/ManagerReservedMemoryUpdate.hpp"
#include "starlight/policy/event/ListenForManagerReservedMemoryUpdate.hpp"

#include <star_common/HandleTypeRegistry.hpp>

#include <memory>

namespace star::core::renderer
{
/// @brief Contains information needed for objects to process their rendering commands
class RenderingContext
{
  public:
    vk::Extent2D targetResolution;
    StarPipeline *pipeline = nullptr;
    MappedHandleContainer<vk::Buffer> bufferTransferRecords =
        MappedHandleContainer<vk::Buffer>(common::special_types::BufferTypeName);
    MappedHandleContainer<vk::Semaphore> recordDependentSemaphores =
        MappedHandleContainer<vk::Semaphore>(common::special_types::SemaphoreTypeName);
    MappedHandleContainer<vk::Fence> recordDependentFence =
        MappedHandleContainer<vk::Fence>(common::special_types::FenceTypeName);
    /// @brief Texture pointers this pass records against, keyed by Image-manager handle.
    /// @details The pointers point into the manager's record storage, which moves as it grows; the listener below
    /// re-resolves every entry when that happens.
    MappedHandleContainer<StarTextures::Texture *> recordDependentImage =
        MappedHandleContainer<StarTextures::Texture *>(common::special_types::GetImageTypeName);

    /// @brief Re-resolve every cached dependent-image pointer after the Image manager's record storage moves.
    void onManagerReservedMemoryUpdate(const star::event::ManagerReservedMemoryUpdate &event, bool &keepAlive);

    using ImageResizeListener = star::policy::event::ListenForManagerReservedMemoryUpdate<RenderingContext>;

    /// @brief Subscribe to Image-manager reallocation events.
    void initListener(common::EventBus &eventBus);
    /// @brief Unsubscribe; safe to call when initListener() was never called.
    void cleanupListener(common::EventBus &eventBus);

    /// @brief Heap-held so the context stays movable through its implicit move operations.
    /// @details Move only before initListener() has run; a moved listener would still reference the old context.
    std::unique_ptr<ImageResizeListener> m_imageResizeListener;

    void addBufferToRenderingContext(core::device::DeviceContext &context, const Handle &handle)
    {
        assert(handle.getType() ==
               common::HandleTypeRegistry::instance().getType(common::special_types::BufferTypeName).value());

        bufferTransferRecords.manualInsert(
            handle, context.getManagerRenderResource().getBuffer(context.getDeviceID(), handle).getVulkanBuffer());
    }
};
} // namespace star::core::renderer
