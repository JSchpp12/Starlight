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

#include <chrono>
#include <memory>
#include <optional>
#include <vulkan/vulkan.hpp>

namespace star::core::renderer
{
class DefaultRenderer : public RendererBase
{
  public:
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

    DefaultRenderer(DefaultRenderer &) = delete;
    DefaultRenderer &operator=(DefaultRenderer &) = delete;
    DefaultRenderer(DefaultRenderer &&other) = default;
    DefaultRenderer &operator=(DefaultRenderer &&other) = default;
    virtual ~DefaultRenderer() = default;

    virtual void prepRender(common::IDeviceContext &device, const uint8_t &numFramesInFlight) override;
    virtual void cleanupRender(common::IDeviceContext &device) override;
    virtual void frameUpdate(common::IDeviceContext &context, const uint8_t &frameInFlightIndex) override;

    StarDescriptorSetLayout &getGlobalShaderInfo()
    {
        return *this->globalSetLayout;
    }

    std::vector<Handle> &getRenderToColorImages()
    {
        return m_renderToImages;
    }
    const std::vector<Handle> &getRenderToColorImages() const
    {
        return m_renderToImages;
    }

    std::vector<Handle> &getRenderToDepthImages()
    {
        return m_renderToDepthImages;
    }
    const std::vector<Handle> &getRenderToDepthImages() const
    {
        return m_renderToDepthImages;
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
    bool ownsRenderResourceControllers = false;
    bool isReady = false;
    std::shared_ptr<ManagerController::RenderResource::Buffer> m_infoManagerLightData, m_infoManagerLightList,
        m_infoManagerCamera;

    std::vector<Handle> m_renderToImages;
    std::vector<Handle> m_renderToDepthImages;

    std::shared_ptr<StarDescriptorSetLayout> globalSetLayout = nullptr;
    core::renderer::RenderingContext m_renderingContext = core::renderer::RenderingContext();
    vk::Format m_colorFormat, m_depthFormat;

    void initBuffers(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                     std::shared_ptr<std::vector<Light>> lights);

    void initBuffers(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                     std::shared_ptr<std::vector<Light>> lights, std::shared_ptr<StarCamera> camera);

    virtual std::vector<StarTextures::Texture> createRenderToImages(core::device::DeviceContext &context,
                                                                    const uint8_t &numFramesInFlight);

    virtual std::vector<StarTextures::Texture> createRenderToDepthImages(core::device::DeviceContext &context,
                                                                         const uint8_t &numFramesInFlight);

    /// Create the descriptor set layout that will be shared by all objects within this renderer
    virtual std::shared_ptr<StarDescriptorSetLayout> createGlobalDescriptorSetLayout(
        core::device::DeviceContext &context, const uint8_t &numFramesInFlight);

    vk::ImageView createImageView(core::device::DeviceContext &device, vk::Image image, vk::Format format,
                                  vk::ImageAspectFlags aspectFlags);

    virtual StarShaderInfo::Builder manualCreateDescriptors(core::device::DeviceContext &device,
                                                            const uint8_t &numFramesInFlight);

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

    virtual void updateDependentData(star::core::device::DeviceContext &context, const uint8_t &frameInFlightIndex);

    virtual core::device::manager::ManagerCommandBuffer::Request getCommandBufferRequest() override = 0;
#pragma region helpers
    vk::Viewport prepareRenderingViewport(const vk::Extent2D &resolution);

    virtual vk::RenderingAttachmentInfo prepareDynamicRenderingInfoColorAttachment(const uint8_t &frameInFlightIndex);

    virtual vk::RenderingAttachmentInfo prepareDynamicRenderingInfoDepthAttachment(const uint8_t &frameInFlightIndex);

    virtual void recordCommandBuffer(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
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

#pragma endregion
  private:
    // Inherited via DescriptorModifier
    std::vector<std::pair<vk::DescriptorType, const int>> getDescriptorRequests(const int &numFramesInFlight);
    void createDescriptors(star::core::device::DeviceContext &device, const int &numFramesInFlight);
};
} // namespace star::core::renderer