#pragma once

#include "Light.hpp"
#include "LightBufferObject.hpp"
#include "ManagerController_RenderResource_Buffer.hpp"
#include "StarCamera.hpp"
#include "StarCommandBuffer.hpp"
#include "StarDescriptorBuilders.hpp"
#include "StarShaderInfo.hpp"
#include "StarTextures/Texture.hpp"
#include "core/renderer/FrameData.hpp"
#include "core/renderer/RenderPhase.hpp"
#include "core/renderer/RenderTargets.hpp"
#include "core/renderer/RenderingContext.hpp"
#include "starlight/event/DescriptorPoolReady.hpp"
#include "starlight/object/StarObject.hpp"

#include <star_common/FrameTracker.hpp>

#include <functional>
#include <memory>
#include <vulkan/vulkan.hpp>

namespace star::core::renderer
{
class DefaultRenderPhaseProvider;

class DefaultRenderPhase : public RenderPhase
{
  public:
    DefaultRenderPhase() = default;
    virtual ~DefaultRenderPhase() = default;
    DefaultRenderPhase(const DefaultRenderPhase &) = delete;
    DefaultRenderPhase &operator=(const DefaultRenderPhase &) = delete;
    DefaultRenderPhase(DefaultRenderPhase &&) = delete;
    DefaultRenderPhase &operator=(DefaultRenderPhase &&) = delete;

    virtual void frameUpdate(common::IDeviceContext &context) override;
    virtual void recordCommandBuffer(StarCommandBuffer &commandBuffer, const common::FrameTracker &frameInFlightIndex,
                                     const uint64_t &frameIndex) override;
    virtual void cleanupRender(common::IDeviceContext &context) override;

    /// This phase owns and drives the named roles (they must be DrivenBuffer
    /// slots in m_frameData): it submits their per-frame CPU->GPU transfer and
    /// emits the transfer->shader read-back barrier each frame.
    DefaultRenderPhase &setDataRolesOwned(Handle cameraRole, Handle lightInfoRole, Handle lightListRole);
    /// This phase only reads the named roles (driven by another phase): no
    /// frameUpdate, no barriers.
    DefaultRenderPhase &setDataRolesBorrowed(Handle cameraRole, Handle lightInfoRole, Handle lightListRole);

  protected:
    struct DataRoles
    {
        Handle camera;
        Handle lightInfo;
        Handle lightList;
    };
    using OwningBarrierFunction = void (*)(uint8_t, const uint64_t &, const FrameData *, const DataRoles *,
                                           const RenderingContext *, vk::BufferMemoryBarrier2 *, size_t *) noexcept;
    friend class DefaultRenderPhaseProvider;

    std::array<vk::BufferMemoryBarrier2, 3> m_runtimeBarriers;
    /// Global descriptor set (camera/lights) owned by the phase and bound once per
    /// frame. Previously this set was duplicated into every material and rebound
    /// for every mesh.
    std::unique_ptr<StarShaderInfo> m_globalShaderInfo;
    DataRoles m_dataRoles{};
    OwningBarrierFunction m_barrFunction{nullptr};
    bool isReady = false;

    virtual void updateDependentData(star::core::device::DeviceContext &context);

    vk::Viewport prepareRenderingViewport(const vk::Extent2D &resolution);

    virtual vk::RenderingAttachmentInfo prepareDynamicRenderingInfoColorAttachment(
        const common::FrameTracker &frameTracker);

    virtual vk::RenderingAttachmentInfo prepareDynamicRenderingInfoDepthAttachment(
        const common::FrameTracker &frameTracker);

    virtual void recordCommands(vk::CommandBuffer &commandBuffer, const common::FrameTracker &frameTracker,
                                const uint64_t &frameIndex);

    /// Override to bind the global descriptor set once per render group per frame
    /// (instead of once per mesh) before delegating to the base group iteration.
    virtual void recordRenderingCalls(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                                      const uint64_t &frameIndex) override;

    void recordCommandBufferDependencies(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                                         const uint64_t &frameIndex);

  private:
    static void AddOwnsAllResourcesBarrier(uint8_t flightIndex, const uint64_t &frameIndex, const FrameData *fd,
                                           const DataRoles *roles, const RenderingContext *rc,
                                           vk::BufferMemoryBarrier2 *data, size_t *dCount) noexcept;
};
} // namespace star::core::renderer
