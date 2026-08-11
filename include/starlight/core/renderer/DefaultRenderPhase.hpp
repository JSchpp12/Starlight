#pragma once

#include "Light.hpp"
#include "LightBufferObject.hpp"
#include "ManagerController_RenderResource_Buffer.hpp"
#include "MapManager.hpp"
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
    class WaitForDescriptorPoolReady
    {
      public:
        WaitForDescriptorPoolReady(
            RenderingTargetInfo renderingInfo,
            std::function<star::StarShaderInfo::Builder(star::core::device::DeviceContext &context)> createDescriptors,
            star::core::device::DeviceContext &context, std::vector<StarRenderGroup> &renderGroups)
            : m_renderingTargetInfo(std::move(renderingInfo)), m_createDescriptors(std::move(createDescriptors)),
              m_context(context), m_renderGroups(renderGroups)
        {
        }
        WaitForDescriptorPoolReady(WaitForDescriptorPoolReady &&other) noexcept
            : m_renderingTargetInfo(std::move(other.m_renderingTargetInfo)),
              m_createDescriptors(std::move(other.m_createDescriptors)), m_context(other.m_context),
              m_renderGroups(other.m_renderGroups)
        {
        }
        WaitForDescriptorPoolReady &operator=(WaitForDescriptorPoolReady &&other) noexcept
        {
            if (this != &other)
            {
                m_renderingTargetInfo = std::move(other.m_renderingTargetInfo);
                m_createDescriptors = std::move(other.m_createDescriptors);
                m_context = std::move(other.m_context);
                m_renderGroups = std::move(other.m_renderGroups);
            }
            return *this;
        }
        int operator()(const star::event::DescriptorPoolReady &event, bool &keepAlive)
        {
            assert(m_createDescriptors);

            auto rendererSet = m_createDescriptors(m_context);
            for (auto &group : m_renderGroups)
            {
                group.onDescriptorPoolReady(m_context, rendererSet, m_renderingTargetInfo);
            }

            return 0;
        }

      private:
        RenderingTargetInfo m_renderingTargetInfo;
        std::function<star::StarShaderInfo::Builder(star::core::device::DeviceContext &context)> m_createDescriptors;
        star::core::device::DeviceContext &m_context;
        std::vector<StarRenderGroup> &m_renderGroups;
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

  protected:
    friend class DefaultRenderPhaseProvider;

    std::shared_ptr<ManagerController::RenderResource::Buffer> m_infoManagerLightData, m_infoManagerLightList,
        m_infoManagerCamera;
    std::shared_ptr<StarDescriptorSetLayout> globalSetLayout;
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

    virtual star::StarShaderInfo::Builder manualCreateDescriptors(star::core::device::DeviceContext &context);

    virtual std::shared_ptr<star::StarDescriptorSetLayout> createGlobalDescriptorSetLayout(
        device::DeviceContext &context, const uint8_t &numFramesInFlight);
};
} // namespace star::core::renderer