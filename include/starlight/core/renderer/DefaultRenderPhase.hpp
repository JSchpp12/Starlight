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
#include "core/renderer/RenderPhaseConfig.hpp"
#include "core/renderer/RenderTargets.hpp"
#include "core/renderer/RenderingContext.hpp"
#include "starlight/event/DescriptorPoolReady.hpp"
#include "starlight/object/StarObject.hpp"

#include <star_common/FrameTracker.hpp>

#include <array>
#include <functional>
#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace star::core::renderer
{
class DefaultRenderPhaseProvider;

class DefaultRenderPhase : public RenderPhase
{
  public:
    class Builder
    {
      public:
        Builder(core::device::DeviceContext &context);
        Builder &setObjects(std::vector<std::shared_ptr<StarObject>> objects);
        Builder &setFrameData(std::shared_ptr<FrameData> frameData);
        Builder &setOwnsFrameData(bool owned);
        Builder &setConfig(RenderPhaseConfig config);
        Builder &setRenderTargetsFactory(std::function<RenderTargets(RenderingContext &)> factory);
        std::unique_ptr<DefaultRenderPhase> buildUnique();
        void buildInto(DefaultRenderPhase &target);

      private:
        std::function<RenderTargets(RenderingContext &)> m_renderTargetsFactory;
        RenderPhaseConfig m_config{};
        std::vector<std::shared_ptr<StarObject>> m_objects;
        std::shared_ptr<FrameData> m_frameData;
        core::device::DeviceContext &m_context;
        bool m_ownsFrameData = false;
    };

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

    /// @brief This phase owns and drives the named roles (they must be DrivenBuffer slots in m_frameData): it submits
    /// their per-frame CPU->GPU transfer and emits the transfer->shader read-back barrier each frame.
    /// @param owned
    /// @return
    DefaultRenderPhase &setDataRoleOwnership(bool owned);

  protected:
    using OwningBarrierFunction = void (*)(const common::FrameTracker &, const uint64_t &, const FrameData *,
                                           const RenderingContext *, vk::BufferMemoryBarrier2 *, size_t *) noexcept;
    friend class Builder;

    std::array<vk::BufferMemoryBarrier2, 3> m_runtimeBarriers;
    /// Global descriptor set (camera/lights) owned by the phase and bound once per
    /// frame. Previously this set was duplicated into every material and rebound
    /// for every mesh.
    std::unique_ptr<StarShaderInfo> m_globalShaderInfo;
    std::array<vk::DescriptorSet, 1> m_descriptors;
    OwningBarrierFunction m_barrFunction{nullptr};
    bool isReady = false;

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

    void recordCommandBufferDependencies(vk::CommandBuffer &commandBuffer, const common::FrameTracker &frameTracker,
                                         const uint64_t &frameIndex);

  private:
    static void AddOwnsAllResourcesBarrier(const common::FrameTracker &frameTracker, const uint64_t &frameIndex,
                                           const FrameData *fd, const RenderingContext *rc,
                                           vk::BufferMemoryBarrier2 *data, size_t *dCount) noexcept;
};
} // namespace star::core::renderer
