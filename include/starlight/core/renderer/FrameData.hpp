#pragma once

#include "ManagerController_RenderResource_Buffer.hpp"
#include "StarBuffers/Buffer.hpp"
#include "StarTextures/Texture.hpp"
#include "core/graphics/GPUWorkSyncInfo.hpp"

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

    /// A registered buffer handle with no per-frame transfer -- the same handle is bound to every frame-in-flight's
    /// descriptor set.
    struct FixedBufferHandle
    {
        Handle handle;
    };

    struct OwnedBuffer
    {
        std::vector<std::shared_ptr<StarBuffers::Buffer>> buffers;
    };

    /// A single texture handle registered with ManagerRenderResource (transfer-
    /// uploaded). Resolved to a texture by StarShaderInfo at bind time. Binding-only.
    struct TextureHandle
    {
        Handle textureHandle;
        vk::ImageLayout layout;
        std::optional<vk::Format> format;
    };

    /// Per-frame-in-flight borrowed render-target textures -- the fi-th texture pointer is bound to the fi-th
    /// descriptor set. For render targets owned by another phase
    struct BorrowedTexture
    {
        std::vector<const StarTextures::Texture *> textures;
        vk::ImageLayout layout;
        std::optional<vk::Format> format;
    };

    /// Per-frame-in-flight textures owned by this phase -- the fi-th texture is bound to the fi-th descriptor set. For
    /// compute-written images the phase itself owns. Binding-only.
    struct OwnedTexture
    {
        std::vector<std::shared_ptr<StarTextures::Texture>> textures;
        vk::ImageLayout layout;
        std::optional<vk::Format> format;
    };

    using Resource = std::variant<DrivenBuffer, BorrowedBuffer, TextureHandle, FixedBufferHandle, OwnedBuffer,
                                  BorrowedTexture, OwnedTexture>;

    FrameData &add(std::shared_ptr<ManagerController::RenderResource::Buffer> controller);
    FrameData &add(std::shared_ptr<ManagerController::RenderResource::Buffer> controller, Handle role);
    FrameData &add(BorrowedBuffer borrowed, Handle role);
    FrameData &add(TextureHandle texture, Handle role);
    FrameData &add(FixedBufferHandle handle, Handle role);
    FrameData &add(OwnedBuffer buffers, Handle role);
    FrameData &add(BorrowedTexture textures, Handle role);
    FrameData &add(OwnedTexture textures, Handle role);

    /// Prep each driven buffer controller (allocates its per-frame-in-flight
    /// handles). Binding-only resources are not touched.
    void prepRender(core::device::DeviceContext &context, uint8_t numFramesInFlight);

    /// Submit each driven buffer controller's per-frame update and collect the sync
    /// the owner must wire. Binding-only resources are not touched.
    FrameUpdateResult frameUpdate(core::device::DeviceContext &context,
                                  std::optional<core::graphics::SemaphoreInfo> priorSync = std::nullopt);
    const Resource &resource(Handle role) const;
    /// The buffer controller registered under `role` (driven or borrowed). Asserts
    /// the slot holds a buffer.
    ManagerController::RenderResource::Buffer *controller(Handle role) const;

  private:
    std::vector<std::pair<Handle, Resource>> m_resources;
    std::unordered_map<Handle, size_t, star::HandleHash> m_roleIndex;
    std::vector<PendingWait> m_pending;
};
} // namespace star::core::renderer