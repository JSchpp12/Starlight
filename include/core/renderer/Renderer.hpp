#pragma once

#include "device/managers/ManagerCommandBuffer.hpp"

#include "CommandBufferModifier.hpp"
#include "DescriptorModifier.hpp"
#include "InteractionSystem.hpp"
#include "Light.hpp"
#include "LightBufferObject.hpp"
#include "ManagerDescriptorPool.hpp"
#include "MapManager.hpp"
#include "RenderResourceModifier.hpp"
#include "StarCamera.hpp"
#include "StarCommandBuffer.hpp"
#include "StarDescriptorBuilders.hpp"
#include "StarObject.hpp"
#include "StarRenderGroup.hpp"
#include "StarShaderInfo.hpp"
#include "StarTextures/Texture.hpp"
#include "StarWindow.hpp"

#include <chrono>
#include <memory>
#include <optional>
#include <vulkan/vulkan.hpp>

namespace star::core::renderer
{
class Renderer : private RenderResourceModifier, private DescriptorModifier
{
  public:
    Renderer(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
             std::vector<std::shared_ptr<Light>> lights, std::vector<Handle> &cameraInfoBuffers,
             std::vector<std::shared_ptr<StarObject>> objects)
        : m_lights(lights), m_cameraInfoBuffers(cameraInfoBuffers)
    {
        initBuffers(context, numFramesInFlight);
        createRenderingGroups(context, context.getRenderingSurface().getResolution(), objects);
    }

    Renderer(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
             std::vector<std::shared_ptr<Light>> lights, std::shared_ptr<StarCamera> camera,
             std::vector<std::shared_ptr<StarObject>> objects)
        : m_lights(lights)
    {
        initBuffers(context, numFramesInFlight, camera);
        createRenderingGroups(context, context.getRenderingSurface().getResolution(), objects);
    }

    Renderer(Renderer &) = delete;
    Renderer &operator=(Renderer &) = delete;
    Renderer(Renderer &&other) = delete;
    Renderer &operator=(Renderer &&other) = delete;
    virtual ~Renderer() = default;

    virtual void prepRender(core::device::DeviceContext &device, const vk::Extent2D &swapChainExtent,
                            const uint8_t &numFramesInFlight);

    virtual void cleanupRender(core::device::DeviceContext &device); 

    virtual void frameUpdate(core::device::DeviceContext &context);

    StarDescriptorSetLayout &getGlobalShaderInfo()
    {
        return *this->globalSetLayout;
    }

    std::vector<std::unique_ptr<StarTextures::Texture>> *getRenderToColorImages()
    {
        return &this->renderToImages;
    }
    std::vector<std::unique_ptr<StarTextures::Texture>> *getRenderToDepthImages()
    {
        return &this->renderToDepthImages;
    }

    virtual RenderingTargetInfo getRenderTargetInfo() const
    {
        return RenderingTargetInfo({this->renderToImages.at(0)->getBaseFormat()},
                                   this->renderToDepthImages.at(0)->getBaseFormat());
    }

    std::vector<Handle> getCameraInfoBuffers()
    {
        return m_cameraInfoBuffers;
    }

    std::vector<Handle> getLightInfoBuffers()
    {
        return m_lightInfoBuffers;
    }

    std::vector<Handle> getLightListBuffers()
    {
        return m_lightListBuffers;
    }
  protected:
    bool isReady = false;
    std::vector<Handle> m_cameraInfoBuffers, m_lightInfoBuffers, m_lightListBuffers;

    Handle m_commandBuffer;
    std::vector<std::shared_ptr<star::Light>> m_lights;

    std::unique_ptr<vk::Extent2D> swapChainExtent = std::unique_ptr<vk::Extent2D>();

    std::vector<std::unique_ptr<StarTextures::Texture>> renderToImages =
        std::vector<std::unique_ptr<StarTextures::Texture>>();
    std::vector<vk::Framebuffer> renderToFramebuffers = std::vector<vk::Framebuffer>();

    // depth testing storage
    std::vector<std::unique_ptr<StarTextures::Texture>> renderToDepthImages =
        std::vector<std::unique_ptr<StarTextures::Texture>>();

    std::shared_ptr<StarDescriptorSetLayout> globalSetLayout = nullptr;
    std::vector<std::unique_ptr<StarRenderGroup>> renderGroups = std::vector<std::unique_ptr<StarRenderGroup>>();

    void initBuffers(core::device::DeviceContext &context, const uint8_t &numFramesInFlight);

    void initBuffers(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                     std::shared_ptr<StarCamera> camera);

    virtual std::vector<std::unique_ptr<StarTextures::Texture>> createRenderToImages(
        core::device::DeviceContext &device, const int &numFramesInFlight);

    virtual std::vector<std::unique_ptr<StarTextures::Texture>> createRenderToDepthImages(
        core::device::DeviceContext &device, const int &numFramesInFlight);

    /// Create the descriptor set layout that will be shared by all objects within this renderer
    virtual std::shared_ptr<StarDescriptorSetLayout> createGlobalDescriptorSetLayout(
        core::device::DeviceContext &context, const uint8_t &numFramesInFlight);

    /// <summary>
    /// Create vertex buffer + index buffers + any rendering groups for operations
    /// </summary>
    void createRenderingGroups(core::device::DeviceContext &device, const vk::Extent2D &swapChainExtent,
                               std::vector<std::shared_ptr<StarObject>> objects);

    vk::ImageView createImageView(core::device::DeviceContext &device, vk::Image image, vk::Format format,
                                  vk::ImageAspectFlags aspectFlags);

    virtual StarShaderInfo::Builder manualCreateDescriptors(core::device::DeviceContext &device,
                                                            const int &numFramesInFlight);

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

    // Inherited via CommandBufferModifier
    virtual core::device::managers::ManagerCommandBuffer::Request getCommandBufferRequest() = 0;

    // Inherited via RenderResourceModifier
    virtual void initResources(core::device::DeviceContext &device, const int &numFramesInFlight,
                               const vk::Extent2D &screensize) override;

    virtual void destroyResources(core::device::DeviceContext &device) override;

    virtual vk::Format getColorAttachmentFormat(star::core::device::DeviceContext &device) const;

    virtual vk::Format getDepthAttachmentFormat(star::core::device::DeviceContext &device) const;

#pragma region helpers
    vk::Viewport prepareRenderingViewport();

    virtual vk::RenderingAttachmentInfo prepareDynamicRenderingInfoColorAttachment(const int &frameInFlightIndex);

    virtual vk::RenderingAttachmentInfo prepareDynamicRenderingInfoDepthAttachment(const int &frameInFlightIndex);

    virtual void recordCommandBuffer(vk::CommandBuffer &commandBuffer, const int &frameInFlightIndex);

    void recordPreRenderingCalls(vk::CommandBuffer &commandBuffer, const int &frameInFlightIndex);

    void recordRenderingCalls(vk::CommandBuffer &commandBuffer, const int &frameInFlightIndex);

    void recordPostRenderingCalls(vk::CommandBuffer &commandBuffer, const int &frameInFlightIndex);

    RenderingTargetInfo getRenderingTargetInfo(core::device::DeviceContext &context)
    {
        return RenderingTargetInfo({this->getColorAttachmentFormat(context)},
                                   {this->getDepthAttachmentFormat(context)});
    }
#pragma endregion
  private:
    // Inherited via DescriptorModifier
    std::vector<std::pair<vk::DescriptorType, const int>> getDescriptorRequests(const int &numFramesInFlight) override;
    void createDescriptors(star::core::device::DeviceContext &device, const int &numFramesInFlight) override;

    void updateRenderingGroups(core::device::DeviceContext &context);
};
} // namespace star::core::renderer