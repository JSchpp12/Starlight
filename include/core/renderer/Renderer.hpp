#pragma once

#include "device/managers/ManagerCommandBuffer.hpp"

#include "CommandBufferModifier.hpp"
#include "DescriptorModifier.hpp"
#include "InteractionSystem.hpp"
#include "Light.hpp"
#include "LightBufferObject.hpp"
#include "LightManager.hpp"
#include "ManagerDescriptorPool.hpp"
#include "MapManager.hpp"
#include "RenderResourceModifier.hpp"
#include "ShaderManager.hpp"
#include "StarCamera.hpp"
#include "StarCommandBuffer.hpp"
#include "StarDescriptorBuilders.hpp"
#include "StarObject.hpp"
#include "StarRenderGroup.hpp"
#include "StarScene.hpp"
#include "StarShaderInfo.hpp"
#include "StarTextures/Texture.hpp"
#include "StarWindow.hpp"

#include <chrono>
#include <memory>
#include <optional>
#include <vulkan/vulkan.hpp>

namespace star
{
class Renderer : private RenderResourceModifier,
                      private DescriptorModifier
{
  public:
    Renderer(std::shared_ptr<StarScene> scene);

    virtual void prepare(core::device::DeviceContext &device, const vk::Extent2D &swapChainExtent, const int &numFramesInFlight);

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

  protected:
    Handle m_commandBuffer; 
    std::shared_ptr<StarScene> scene = nullptr;

    std::unique_ptr<vk::Extent2D> swapChainExtent = std::unique_ptr<vk::Extent2D>();

    std::vector<std::unique_ptr<StarTextures::Texture>> renderToImages =
        std::vector<std::unique_ptr<StarTextures::Texture>>();
    std::vector<vk::Framebuffer> renderToFramebuffers = std::vector<vk::Framebuffer>();

    // depth testing storage
    std::vector<std::unique_ptr<StarTextures::Texture>> renderToDepthImages =
        std::vector<std::unique_ptr<StarTextures::Texture>>();

    // storage for multiple buffers for each swap chain image
    // std::vector<vk::DescriptorSet> globalDescriptorSets;
    std::unique_ptr<StarShaderInfo> globalShaderInfo = nullptr;
    std::vector<vk::DescriptorSet> lightDescriptorSets = std::vector<vk::DescriptorSet>();

    std::shared_ptr<StarDescriptorSetLayout> globalSetLayout = nullptr;
    std::vector<std::unique_ptr<StarRenderGroup>> renderGroups = std::vector<std::unique_ptr<StarRenderGroup>>();

    virtual std::vector<std::unique_ptr<StarTextures::Texture>> createRenderToImages(core::device::DeviceContext &device,
                                                                                     const int &numFramesInFlight);

    virtual std::vector<std::unique_ptr<StarTextures::Texture>> createRenderToDepthImages(core::device::DeviceContext &device,
                                                                                          const int &numFramesInFlight);

    /// <summary>
    /// Create vertex buffer + index buffers + any rendering groups for operations
    /// </summary>
    virtual void createRenderingGroups(core::device::DeviceContext &device, const vk::Extent2D &swapChainExtent,
                                       const int &numFramesInFlight, StarShaderInfo::Builder builder);

    vk::ImageView createImageView(core::device::DeviceContext &device, vk::Image image, vk::Format format,
                                  vk::ImageAspectFlags aspectFlags);

    virtual StarShaderInfo::Builder manualCreateDescriptors(core::device::DeviceContext &device, const int &numFramesInFlight);

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

    virtual void prepareForSubmission(const int &frameIndexToBeDrawn);

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
#pragma endregion
  private:
    // Inherited via DescriptorModifier
    std::vector<std::pair<vk::DescriptorType, const int>> getDescriptorRequests(const int &numFramesInFlight) override;
    void createDescriptors(star::core::device::DeviceContext &device, const int &numFramesInFlight) override;
};
} // namespace star