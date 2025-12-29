#pragma once

#include "StarCommandBuffer.hpp"
#include "StarEntity.hpp"
#include "StarGraphicsPipeline.hpp"
#include "StarMaterial.hpp"
#include "StarMesh.hpp"
#include "StarObjectInstance.hpp"
#include "StarPipeline.hpp"
#include "StarShader.hpp"
#include "StarShaderInfo.hpp"
#include "core/device/DeviceContext.hpp"
#include "core/renderer/RenderingContext.hpp"

#include "ManagerController_RenderResource_InstanceModelInfo.hpp"
#include "ManagerController_RenderResource_InstanceNormalInfo.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace star
{
/// <summary>
/// Base class for renderable objects.
/// </summary>
class StarObject
{
  public:
    bool drawNormals = false;
    bool drawBoundingBox = false;
    bool isVisible = true;

    static void initSharedResources(core::device::DeviceContext &device, vk::Extent2D swapChainExtent,
                                    int numSwapChainImages, StarDescriptorSetLayout &globalDescriptors,
                                    core::renderer::RenderingTargetInfo renderingInfo);

    static void cleanupSharedResources(core::device::DeviceContext &device);

    StarObject() = default;

    explicit StarObject(std::vector<std::shared_ptr<StarMaterial>> meshMaterials)
        : m_meshMaterials(std::move(meshMaterials)){};

    virtual ~StarObject() = default;

    virtual void init(core::device::DeviceContext &device);

    virtual void cleanupRender(core::device::DeviceContext &device);

    virtual Handle buildPipeline(core::device::DeviceContext &device, vk::Extent2D swapChainExtent,
                                 vk::PipelineLayout pipelineLayout, core::renderer::RenderingTargetInfo renderInfo);

    virtual void prepRender(star::core::device::DeviceContext &context, const vk::Extent2D &swapChainExtent,
                            const uint8_t &numSwapChainImages, StarShaderInfo::Builder fullEngineBuilder,
                            vk::PipelineLayout pipelineLayout, core::renderer::RenderingTargetInfo renderingInfo);

    virtual void prepRender(star::core::device::DeviceContext &context, const vk::Extent2D &swapChainExtent,
                            const uint8_t &numSwapChainImages, star::StarShaderInfo::Builder fullEngineBuilder,
                            Handle sharedPipeline);

    virtual core::renderer::RenderingContext buildRenderingContext(star::core::device::DeviceContext &context);

    /// Function to contain any commands to be submitted before the start of the rendering pass this object is contained
    /// in begins
    virtual void recordPreRenderPassCommands(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                                             const uint64_t &frameIndex);

    /// Function to contain any commands to be submitted after the end of the rendering pass this object is contained in
    virtual void recordPostRenderPassCommands(vk::CommandBuffer &commandBuffer, const int &frameInFlightIndex) {};

    /// <summary>
    /// Create render call
    /// </summary>
    /// <param name="commandBuffer"></param>
    /// <param name="pipelineLayout"></param>
    /// <param name="swapChainIndexNum"></param>
    void recordRenderPassCommands(vk::CommandBuffer &commandBuffer, vk::PipelineLayout &pipelineLayout,
                                  const uint8_t &swapChainIndexNum, const uint64_t &frameIndex);

    /// @brief Create an instance of this object.
    /// @return A reference to the created instance. The object will own the instance.
    StarObjectInstance &createInstance();

    StarObjectInstance &getInstance(const size_t &index = 0);
    const StarObjectInstance &getInstance(const size_t &index = 0) const;

    virtual void frameUpdate(core::device::DeviceContext &context, const uint8_t &frameInFlightIndex,
                             const Handle &targetCommandBuffer);

    /// @brief Create descriptor set layouts for this object.
    /// @param device
    /// @return
    virtual std::vector<std::shared_ptr<star::StarDescriptorSetLayout>> getDescriptorSetLayouts(
        core::device::DeviceContext &device);

    /// <summary>
    /// Every material must provide a method to return shaders within a map. The keys of the map will contain the stages
    /// in which the corresponding shader will be used.
    /// </summary>
    /// <returns></returns>
    virtual std::unordered_map<star::Shader_Stage, StarShader> getShaders() = 0;
#pragma region getters
    Handle &getPipline()
    {
        return this->pipeline;
    }
    const Handle &getPipeline() const
    {
        return this->pipeline;
    }
#pragma endregion

  protected:
    class InstanceInfo
    {
      public:
        InstanceInfo()
            : m_instances(std::make_shared<std::vector<StarObjectInstance>>()),
              m_infoManagerInstanceModel(
                  std::make_shared<ManagerController::RenderResource::InstanceModelInfo>(m_instances)),
              m_infoManagerInstanceNormal(
                  std::make_shared<ManagerController::RenderResource::InstanceNormalInfo>(m_instances))
        {
        }

        void prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight)
        {
            m_infoManagerInstanceModel->prepRender(context, numFramesInFlight);
            m_infoManagerInstanceNormal->prepRender(context, numFramesInFlight);
        }

        size_t getSize()
        {
            return m_instances->size();
        }

        StarObjectInstance &create()
        {
            m_instances->emplace_back();
            return m_instances->back();
        }

        StarObjectInstance &getInstance(const size_t &index)
        {
            assert(index < m_instances->size());
            setManagersToUpdate();

            return m_instances->at(index);
        }
        const StarObjectInstance &getInstance(const size_t &index) const
        {
            assert(index < m_instances->size());
            return m_instances->at(index);
        }
        std::shared_ptr<ManagerController::RenderResource::InstanceModelInfo> getControllerModel()
        {
            return m_infoManagerInstanceModel;
        }
        std::shared_ptr<ManagerController::RenderResource::InstanceNormalInfo> getControllerNormal()
        {
            return m_infoManagerInstanceNormal;
        }

      private:
        std::shared_ptr<std::vector<StarObjectInstance>> m_instances = nullptr;
        std::shared_ptr<ManagerController::RenderResource::InstanceModelInfo> m_infoManagerInstanceModel = nullptr;
        std::shared_ptr<ManagerController::RenderResource::InstanceNormalInfo> m_infoManagerInstanceNormal = nullptr;

        void setManagersToUpdate()
        {
            m_infoManagerInstanceModel->setToUpdate();
            m_infoManagerInstanceNormal->setForUpdate();
        }
    };
    /// pipeline + rendering infos
    // Pipeline* sharedPipeline = nullptr;
    std::vector<std::shared_ptr<StarMaterial>> m_meshMaterials = std::vector<std::shared_ptr<StarMaterial>>();
    Handle pipeline, sharedPipeline;
    core::renderer::RenderingContext renderingContext;
    std::unique_ptr<StarPipeline> normalExtrusionPipeline;
    std::unique_ptr<StarDescriptorSetLayout> setLayout;
    InstanceInfo m_instanceInfo;

    std::vector<std::unique_ptr<StarMesh>> meshes;
    bool isReady = false;

    virtual std::vector<std::unique_ptr<StarMesh>> loadMeshes(star::core::device::DeviceContext &device) = 0;

    virtual void createInstanceBuffers(star::core::device::DeviceContext &context, const uint8_t &numImagesInFlight);

    virtual void createBoundingBox(std::vector<Vertex> &verts, std::vector<uint32_t> &inds);

    std::vector<std::pair<vk::DescriptorType, const uint32_t>> getDescriptorRequests(const uint8_t &numFramesInFlight);
    virtual bool isRenderReady(core::device::DeviceContext &context);

    virtual void updateDependentData(core::device::DeviceContext &context, const uint8_t &frameInFlightIndex,
                                     const Handle &targetCommandBuffer);

  private:
    static std::unique_ptr<StarDescriptorSetLayout> instanceDescriptorLayout;
    static vk::PipelineLayout extrusionPipelineLayout;
    static std::unique_ptr<Handle> tri_normalExtrusionPipeline, triAdj_normalExtrusionPipeline;
    static std::unique_ptr<StarDescriptorSetLayout> boundDescriptorLayout;
    static vk::PipelineLayout boundPipelineLayout;
    static std::unique_ptr<Handle> boundBoxPipeline;

    Handle m_deviceID;
    std::unique_ptr<std::vector<std::reference_wrapper<StarDescriptorSetLayout>>> groupLayout;
    std::unique_ptr<std::vector<std::vector<vk::DescriptorSet>>> globalSets;

    Handle boundingBoxVertBuffer, boundingBoxIndexBuffer;
    std::vector<std::vector<vk::DescriptorSet>> boundingDescriptors;
    Handle vertBuffer, indBuffer;
    uint32_t boundingBoxIndsCount = 0;

    void prepStarObject(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                        StarShaderInfo::Builder &frameBuilder);

    void prepMaterials(star::core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                       StarShaderInfo::Builder &frameBuilder);

    void recordDrawCommandNormals(vk::CommandBuffer &commandBuffer);

    void recordDrawCommandBoundingBox(vk::CommandBuffer &commandBuffer, int inFlightIndex);

    void calculateBoundingBox(std::vector<Vertex> &verts, std::vector<uint32_t> &inds);

    void prepareMeshes(star::core::device::DeviceContext &context);

    void updateInstanceData(core::device::DeviceContext &context, const uint8_t &frameInFlightIndex,
                            const Handle &targetCommandBuffer);

    bool isKnownToBeReadyForRecordRender(const uint8_t &frameInFlightIndex) const;

    void recordDependentDataPipelineBarriers(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                                             const uint64_t &frameIndex);
};
} // namespace star