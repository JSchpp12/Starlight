#pragma once

#include "StarDescriptorBuilders.hpp"
#include "StarShaderInfo.hpp"
#include "core/device/DeviceContext.hpp"
#include "starlight/object/StarObject.hpp"
#include <star_common/Handle.hpp>

#include <vulkan/vulkan.hpp>

#include <memory>
#include <vector>

namespace star
{
/// @brief Class which contains a group of objects which share pipeline dependencies which are compatible with one
/// another. Implies that they can all share same pipeline layout but NOT the same pipeline.
class StarRenderGroup
{
  public:
    StarRenderGroup(core::device::DeviceContext &device, std::shared_ptr<StarObject> baseObject);
    StarRenderGroup(StarRenderGroup &&) = default;
    StarRenderGroup &operator=(StarRenderGroup &&) = default;
    StarRenderGroup(const StarRenderGroup &) = default;
    StarRenderGroup &operator=(const StarRenderGroup &) = default;
    ~StarRenderGroup() = default;

    void prepRender(core::device::DeviceContext &context);

    void cleanupRender(core::device::DeviceContext &context);

    bool isObjectCompatible(StarObject &object, core::device::DeviceContext &context);

    void addObject(std::shared_ptr<StarObject> newRenderObject, core::device::DeviceContext &context);

    void onDescriptorPoolReady(star::core::device::DeviceContext &context, StarShaderInfo::Builder initEngineBuilder,
                               star::core::renderer::RenderingTargetInfo &rendererInfo, star::Handle commandBuffer);

    vk::PipelineLayout getPipelineLayout() const
    {
        return m_pipelineLayout;
    }

    void frameUpdate(core::device::DeviceContext &context, uint8_t frameInFlightIndex, Handle targetCommandBuffer,
                     const star::core::graphics::SemaphoreInfo &transferReuqestSyncInfo);

    void recordRenderPassCommands(vk::CommandBuffer mainDrawBuffer, uint8_t swapChainImageIndex, uint64_t frameIndex);

    /// @param recordingInfo Recording submission of the phase's command buffer, or null when no object in the phase
    /// has a pending setup and no completion gate is needed.
    void recordPreRenderPassCommands(vk::CommandBuffer mainDrawBuffer, const common::FrameTracker &frameTracker,
                                     uint64_t frameIndex, const core::graphics::RecordingSubmissionInfo *recordingInfo);

    void recordPostRenderPassCommands(vk::CommandBuffer commandBuffer, int frameInFlightIndex);

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
    std::vector<std::shared_ptr<StarDescriptorSetLayout>> m_largestDescriptorSet;
    std::vector<Group> m_groups;
    vk::PipelineLayout m_pipelineLayout{VK_NULL_HANDLE};

    void prepareObjects(star::core::device::DeviceContext &context);

    vk::PipelineLayout createPipelineLayout(core::device::DeviceContext &context,
                                            std::vector<std::shared_ptr<StarDescriptorSetLayout>> &fullSetLayout);
};
} // namespace star
