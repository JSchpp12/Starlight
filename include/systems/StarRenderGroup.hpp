#pragma once

#include "CastHelpers.hpp"
#include "DescriptorModifier.hpp"
#include "DeviceContext.hpp"
#include "Enums.hpp"
#include "Handle.hpp"
#include "Light.hpp"
#include "StarCommandBuffer.hpp"
#include "StarDescriptorBuilders.hpp"
#include "StarGraphicsPipeline.hpp"
#include "StarObject.hpp"
#include "StarShader.hpp"
#include "StarShaderInfo.hpp"
#include "VulkanVertex.hpp"

#include <vulkan/vulkan.hpp>

#include <map>
#include <memory>
#include <vector>

namespace star
{
/// <summary>
/// Class which contains a group of objects which share pipeline dependencies
/// which are compatible with one another. Implies that they can all share
/// same pipeline layout but NOT the same pipeline.
/// </summary>
class StarRenderGroup
{
  public:
    StarRenderGroup(core::device::DeviceContext &device, std::shared_ptr<StarObject> baseObject);

    // no copy
    StarRenderGroup(const StarRenderGroup &baseObject) = delete;

    virtual ~StarRenderGroup();

    void prepRender(core::device::DeviceContext &context, const vk::Extent2D &renderingResolution,
                    const uint8_t &numFramesInFlight, StarShaderInfo::Builder initEngineBuilder,
                    core::renderer::RenderingTargetInfo renderingInfo);

    void cleanupRender(core::device::DeviceContext &context); 
    
    virtual bool isObjectCompatible(StarObject &object);

    /// <summary>
    /// Register a light color and location for rendering light effects on objects
    /// </summary>
    virtual void addObject(std::shared_ptr<StarObject> newRenderObject);

    virtual void addLight(Light *newLight)
    {
        this->lights.push_back(newLight);
    }

    void frameUpdate(core::device::DeviceContext &context);

    virtual void recordRenderPassCommands(vk::CommandBuffer &mainDrawBuffer, const int &swapChainImageIndex);

    virtual void recordPreRenderPassCommands(vk::CommandBuffer &mainDrawBuffer, const int &swapChainImageIndex);

    virtual void recordPostRenderPassCommands(vk::CommandBuffer &commandBuffer, const int &frameInFlightIndex);

    int getNumObjects()
    {
        return numObjects;
    }

  protected:
    struct RenderObjectInfo
    {
        std::shared_ptr<StarObject> object = nullptr;
        uint32_t startVBIndex = 0, startIBIndex = 0;
    };

    /// <summary>
    /// Groups of objects that can share a pipeline -- they have the same shader
    /// </summary>
    struct Group
    {
        RenderObjectInfo baseObject;
        std::vector<RenderObjectInfo> objects;
    };
    core::device::DeviceContext &device;

    int numObjects = 0;

    vk::PipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    std::vector<std::shared_ptr<StarDescriptorSetLayout>> largestDescriptorSet;

    std::vector<Light *> lights;

    std::vector<Group> groups;

    /// <summary>
    /// Create descriptors for binding render buffers to shaders.
    /// </summary>
    void prepareObjects(StarShaderInfo::Builder &groupBuilder, core::renderer::RenderingTargetInfo renderingInfo,
                        const vk::Extent2D &swapChainExtent, const uint8_t &numFramesInFlight);

    virtual vk::PipelineLayout createPipelineLayout(core::device::DeviceContext &context, std::vector<std::shared_ptr<StarDescriptorSetLayout>> &fullSetLayout);
};
} // namespace star