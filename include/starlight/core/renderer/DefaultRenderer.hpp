#pragma once

#include "Light.hpp"
#include "LightBufferObject.hpp"
#include "ManagerController_RenderResource_Buffer.hpp"
#include "MapManager.hpp"
#include "StarCamera.hpp"
#include "StarCommandBuffer.hpp"
#include "StarDescriptorBuilders.hpp"
#include "StarObject.hpp"
#include "StarShaderInfo.hpp"
#include "StarTextures/Texture.hpp"
#include "core/renderer/RendererBase.hpp"
#include "starlight/event/DescriptorPoolReady.hpp"

#include <star_common/FrameTracker.hpp>

#include <chrono>
#include <memory>
#include <optional>
#include <vulkan/vulkan.hpp>

namespace star::core::renderer
{
class DefaultRenderer : public RendererBase
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

    DefaultRenderer() = default;
    DefaultRenderer(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                    std::shared_ptr<std::vector<Light>> lights, std::shared_ptr<StarCamera> camera,
                    std::vector<std::shared_ptr<StarObject>> objects)
        : RendererBase(context, numFramesInFlight, std::move(objects)), ownsRenderResourceControllers(true)
    {
        initBuffers(context, numFramesInFlight, std::move(lights), camera);
    }

    DefaultRenderer(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                    std::vector<std::shared_ptr<StarObject>> objects,
                    std::shared_ptr<ManagerController::RenderResource::Buffer> lightData,
                    std::shared_ptr<ManagerController::RenderResource::Buffer> lightListData,
                    std::shared_ptr<ManagerController::RenderResource::Buffer> cameraData)
        : RendererBase(context, numFramesInFlight, std::move(objects)), m_infoManagerLightData(std::move(lightData)),
          ownsRenderResourceControllers(false), m_infoManagerLightList(std::move(lightListData)),
          m_infoManagerCamera(std::move(cameraData))
    {
    }

    DefaultRenderer(const DefaultRenderer &) = delete;
    DefaultRenderer &operator=(const DefaultRenderer &) = delete;
    DefaultRenderer(DefaultRenderer &&other) = default;
    DefaultRenderer &operator=(DefaultRenderer &&other) = default;
    virtual ~DefaultRenderer() = default;

    virtual void prepRender(common::IDeviceContext &device) override;
    virtual void cleanupRender(common::IDeviceContext &device) override;
    virtual void frameUpdate(common::IDeviceContext &context) override;

    StarDescriptorSetLayout &getGlobalShaderInfo()
    {
        return *this->globalSetLayout;
    }

    virtual RenderingTargetInfo getRenderTargetInfo() const
    {
        return RenderingTargetInfo({m_colorFormat}, m_depthFormat);
    }

    std::shared_ptr<ManagerController::RenderResource::Buffer> getCameraInfoBuffers()
    {
        return m_infoManagerCamera;
    }

    std::shared_ptr<ManagerController::RenderResource::Buffer> getLightInfoBuffers()
    {
        return m_infoManagerLightData;
    }

    std::shared_ptr<ManagerController::RenderResource::Buffer> getLightListBuffers()
    {
        return m_infoManagerLightList;
    }

  protected:
    core::renderer::RenderingContext m_renderingContext;
    std::shared_ptr<ManagerController::RenderResource::Buffer> m_infoManagerLightData, m_infoManagerLightList,
        m_infoManagerCamera;
    std::shared_ptr<StarDescriptorSetLayout> globalSetLayout;
    vk::Format m_colorFormat, m_depthFormat;
    bool ownsRenderResourceControllers = false;
    bool isReady = false;

    void initBuffers(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                     std::shared_ptr<std::vector<Light>> lights);

    void initBuffers(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                     std::shared_ptr<std::vector<Light>> lights, std::shared_ptr<StarCamera> camera);

    virtual std::vector<StarTextures::Texture> createRenderToImages(core::device::DeviceContext &context,
                                                                    const uint8_t &numFramesInFlight);

    virtual std::vector<StarTextures::Texture> createRenderToDepthImages(core::device::DeviceContext &context,
                                                                         const uint8_t &numFramesInFlight);

    vk::ImageView createImageView(core::device::DeviceContext &device, vk::Image image, vk::Format format,
                                  vk::ImageAspectFlags aspectFlags);

    // virtual StarShaderInfo::Builder manualCreateDescriptors(core::device::DeviceContext &device,
    //                                                         const uint8_t &numFramesInFlight);

    /// <summary>
    /// Create Vulkan Image object with properties provided in function arguments.
    /// </summary>
    /// <param name="width">Width of the image being created</param>
    /// <param name="height">Height of the image being created</param>
    /// <param name="format"></param>
    /// <param name="tiling"></param>
    /// <param name="usage"></param>
    /// <param name="properties"></param>
    /// <param name="image"></param>
    /// <param name="imageMemory"></param>
    virtual void createImage(core::device::DeviceContext &device, uint32_t width, uint32_t height, vk::Format format,
                             vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties,
                             vk::Image &image, VmaAllocation &imageMemory);

    virtual vk::Format getColorAttachmentFormat(star::core::device::DeviceContext &context) const;

    virtual vk::Format getDepthAttachmentFormat(star::core::device::DeviceContext &context) const;

    virtual void updateDependentData(star::core::device::DeviceContext &context);

    virtual core::device::manager::ManagerCommandBuffer::Request getCommandBufferRequest() override = 0;
#pragma region helpers
    vk::Viewport prepareRenderingViewport(const vk::Extent2D &resolution);

    virtual vk::RenderingAttachmentInfo prepareDynamicRenderingInfoColorAttachment(
        const common::FrameTracker &frameTracker);

    virtual vk::RenderingAttachmentInfo prepareDynamicRenderingInfoDepthAttachment(
        const common::FrameTracker &frameTracker);

    virtual void recordCommandBuffer(StarCommandBuffer &commandBuffer, const common::FrameTracker &frameInFlightIndex,
                                     const uint64_t &frameIndex);

    virtual star::StarShaderInfo::Builder manualCreateDescriptors(star::core::device::DeviceContext &context);
    /**
     * @brief Target functionfrom recordCommandBuffer() but without the start and end
     *
     * @param buffer
     * @param flightTracker
     * @param frameIndex
     */
    virtual void recordCommands(vk::CommandBuffer &commandBuffer, const common::FrameTracker &frameTracker,
                                const uint64_t &frameIndex);

    void recordCommandBufferDependencies(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                                         const uint64_t &frameIndex);

    std::vector<vk::BufferMemoryBarrier2> getMemoryBarriersForThisFrame(const uint8_t &frameInFlightIndex,
                                                                        const uint64_t &frameIndex);

    RenderingTargetInfo getRenderingTargetInfo(core::device::DeviceContext &context) const
    {
        return RenderingTargetInfo(std::vector<vk::Format>{this->getColorAttachmentFormat(context)},
                                   this->getDepthAttachmentFormat(context));
    }

    virtual std::shared_ptr<star::StarDescriptorSetLayout> createGlobalDescriptorSetLayout(
        device::DeviceContext &context, const uint8_t &numFramesInFlight);

#pragma endregion
  private:
    std::vector<std::pair<vk::DescriptorType, const int>> getDescriptorRequests(const int &numFramesInFlight);
};
} // namespace star::core::renderer