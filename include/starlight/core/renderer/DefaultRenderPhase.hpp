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

  protected:
    friend class DefaultRenderPhaseProvider;

    /// Role handles for the global shared resources (camera/lightInfo/lightList)
    /// resolved from FrameData. Lightweight value keys -- not controllers.
    Handle m_cameraRole;
    Handle m_lightInfoRole;
    Handle m_lightListRole;
    /// Global descriptor set (camera/lights) owned by the phase and bound once per
    /// frame. Previously this set was duplicated into every material and rebound
    /// for every mesh.
    std::unique_ptr<StarShaderInfo> m_globalShaderInfo;
    bool ownsRenderResourceControllers = false;
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

    std::vector<vk::BufferMemoryBarrier2> getMemoryBarriersForThisFrame(const uint8_t &frameInFlightIndex,
                                                                        const uint64_t &frameIndex);
};
} // namespace star::core::renderer
