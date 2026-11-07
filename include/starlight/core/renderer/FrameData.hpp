#pragma once

#include "ManagerController_RenderResource_Buffer.hpp"
#include "StarBuffers/Buffer.hpp"
#include "StarTextures/Texture.hpp"
#include "core/graphics/GPUWorkSyncInfo.hpp"

#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <star_common/Handle.hpp>
#include <star_common/HandleTypeRegistry.hpp>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace star::core::device
{
class DeviceContext;
}

namespace star::core::renderer
{
namespace frame_roles
{
constexpr std::string_view Camera = "Camera";
constexpr std::string_view LightInfo = "LightInfo";
constexpr std::string_view LightList = "LightList";
} // namespace frame_roles

Handle roleHandle(std::string_view roleName);

class FrameData
{
  public:
    /// Per-phase sync info for a resource that was updated this frame; the phase
    /// must wire these waits before recording against the updated resource.
    struct PendingWait
    {
        Handle handle;
        uint64_t signalValue;
        vk::Semaphore semaphore;
        vk::PipelineStageFlags waitStage;
    };

    struct FrameUpdateResult
    {
        std::span<const PendingWait> waits;
    };

    /// A buffer controller this FrameData OWNS and drives each frame: prepRender
    /// allocates its per-frame-in-flight handles, frameUpdate submits its CPU->GPU transfer.
    struct DrivenBuffer
    {
        std::shared_ptr<ManagerController::RenderResource::Buffer> controller;
    };

    /// A buffer controller this FrameData does NOT own or drive -- it is owned and uploaded by another phase/object.
    /// Held only so it can be role-addressed for binding.
    struct BorrowedBuffer
    {
        ManagerController::RenderResource::Buffer *controller{nullptr};
    };

    /// One handle bound to every frame-in-flight's descriptor set, or one per frame-in-flight (fi-th handle -> fi-th
    /// set). The fixed form stores a bare Handle -- no vector is allocated.
    class HandleSet
    {
      public:
        explicit HandleSet(Handle handle) : m_handles(handle)
        {
        }

        explicit HandleSet(std::vector<Handle> handles) : m_handles(std::move(handles))
        {
        }

        /// @brief True when a single handle is bound to every frame-in-flight's descriptor set.
        bool isFixed() const noexcept
        {
            return std::holds_alternative<Handle>(m_handles);
        }

        /// @brief The handle bound in `frameInFlight`'s descriptor set. A single handle is returned for every frame.
        Handle handleFor(uint8_t frameInFlight) const noexcept
        {
            if (const auto *fixed = std::get_if<Handle>(&m_handles))
            {
                return *fixed;
            }

            const auto &perFrame = std::get<std::vector<Handle>>(m_handles);
            assert(frameInFlight < perFrame.size() && "Frame in flight is out of range for handles");
            return perFrame[frameInFlight];
        }

      private:
        std::variant<Handle, std::vector<Handle>> m_handles;
    };

    /// A buffer handle held by a manager -- either ManagerRenderResource or the device's Buffer manager. Resolved
    /// by handle type at descriptor-write time, so manager record storage may move underneath it. Construct from a
    /// single Handle to bind that buffer to every frame-in-flight's descriptor set, or from a per-frame-in-flight
    /// vector to bind the fi-th handle to the fi-th set. Binding-only.
    class BufferHandle
    {
      public:
        explicit BufferHandle(Handle handle) : m_handles(handle)
        {
        }

        explicit BufferHandle(std::vector<Handle> handles) : m_handles(std::move(handles))
        {
        }

        bool isFixed() const noexcept
        {
            return m_handles.isFixed();
        }

        Handle handleFor(uint8_t frameInFlight) const noexcept
        {
            return m_handles.handleFor(frameInFlight);
        }

      private:
        HandleSet m_handles;
    };

    /// A texture handle held by a manager -- either ManagerRenderResource (transfer-uploaded) or the device's Image
    /// manager. Resolved by handle type when the descriptor set is written, so manager record storage may move
    /// underneath it. Construct from a single Handle to bind that texture to every frame-in-flight's descriptor set,
    /// or from a per-frame-in-flight vector to bind the fi-th handle to the fi-th set. Binding-only.
    class TextureHandle
    {
      public:
        TextureHandle(Handle handle, vk::ImageLayout layout, std::optional<vk::Format> format = std::nullopt)
            : m_handles(handle), layout(layout), format(format)
        {
        }

        TextureHandle(std::vector<Handle> handles, vk::ImageLayout layout,
                      std::optional<vk::Format> format = std::nullopt)
            : m_handles(std::move(handles)), layout(layout), format(format)
        {
        }

        bool isFixed() const noexcept
        {
            return m_handles.isFixed();
        }

        Handle handleFor(uint8_t frameInFlight) const noexcept
        {
            return m_handles.handleFor(frameInFlight);
        }

        vk::ImageLayout layout;
        std::optional<vk::Format> format;

      private:
        HandleSet m_handles;
    };

    using Resource = std::variant<DrivenBuffer, BorrowedBuffer, BufferHandle, TextureHandle>;

    FrameData &add(std::shared_ptr<ManagerController::RenderResource::Buffer> controller);
    FrameData &add(std::shared_ptr<ManagerController::RenderResource::Buffer> controller, Handle role);
    FrameData &add(BorrowedBuffer borrowed, Handle role);
    FrameData &add(BufferHandle handle, Handle role);
    FrameData &add(TextureHandle texture, Handle role);

    /// Prep each driven buffer controller (allocates its per-frame-in-flight
    /// handles). Binding-only resources are not touched.
    void prepRender(core::device::DeviceContext &context, uint8_t numFramesInFlight);

    /// Submit each driven buffer controller's per-frame update and collect the sync
    /// the owner must wire. Binding-only resources are not touched.
    FrameUpdateResult frameUpdate(core::device::DeviceContext &context,
                                  std::optional<core::graphics::SemaphoreInfo> priorSync = std::nullopt);
    const Resource *resource(Handle role) const noexcept;
    bool isResourceDriven(Handle role) const noexcept;
    /// The buffer controller registered under `role` (driven or borrowed). Asserts
    /// the slot holds a buffer.
    const ManagerController::RenderResource::Buffer *getController(Handle role) const noexcept;
    ManagerController::RenderResource::Buffer *getController(Handle role) noexcept;

  private:
    std::vector<std::pair<Handle, Resource>> m_resources;
    std::unordered_map<Handle, size_t, star::HandleHash> m_roleIndex;
    std::vector<PendingWait> m_pending;
};
} // namespace star::core::renderer