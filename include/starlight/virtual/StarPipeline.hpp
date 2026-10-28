#pragma once

#include "StarShader.hpp"
#include "VulkanVertex.hpp"
#include "core/device/StarDevice.hpp"
#include "core/renderer/RenderingTargetInfo.hpp"
#include "wrappers/graphics/StarPipelineConstants.hpp"

#include <absl/container/flat_hash_map.h>
#include <star_common/Handle.hpp>
#include <vulkan/vulkan.hpp>

#include <optional>
#include <vector>

namespace star
{

class StarPipeline;

/// Type of pipeline a PipelineProvider will build.
enum class PipelineType
{
    Graphics,
    Compute
};

struct VertexInputState
{
    std::vector<vk::VertexInputBindingDescription> bindings;
    std::vector<vk::VertexInputAttributeDescription> attributes;
};

struct GraphicsOverrides
{
    VertexInputState vertexInput;
    std::optional<vk::PrimitiveTopology> topology;
    std::optional<vk::CullModeFlags> cullMode;
    std::optional<vk::FrontFace> frontFace;
    std::optional<vk::PipelineDepthStencilStateCreateInfo> depthStencil;
    std::optional<vk::PipelineColorBlendAttachmentState> colorBlendAttachment;
    std::vector<vk::DynamicState> dynamicStates;
};

class PipelineProvider
{
  public:
    struct RenderResourceDependencies
    {
        std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
        core::renderer::RenderingTargetInfo renderingTargetInfo;
        vk::Extent2D swapChainExtent;
    };

    /// Builds a PipelineProvider describing either a graphics or a compute pipeline.
    class Builder
    {
      public:
        /// Defaults to a graphics pipeline: engine-default state + the global VulkanVertex layout.
        Builder() = default;

        /// Shader stages for a graphics pipeline.
        Builder &setShaders(std::vector<Handle> shaders);

        /// Single compute shader. Selects a compute pipeline.
        Builder &setComputeShader(Handle shader);

        Builder &setPipelineLayout(vk::PipelineLayout layout);

        /// Custom vertex input / state overrides for a graphics pipeline.
        Builder &setGraphicsOverrides(GraphicsOverrides overrides);

        Builder &setPipelineConstantsForStage(Shader_Stage stage, StarPipelineConstants constants);

        PipelineProvider build();

      private:
        std::vector<Handle> m_shaders;
        absl::flat_hash_map<Shader_Stage, star::StarPipelineConstants> m_pipelineConstants;
        vk::PipelineLayout m_layout{VK_NULL_HANDLE};
        GraphicsOverrides m_graphics;
        PipelineType m_type{PipelineType::Graphics};
    };

    friend class Builder;

    PipelineProvider() = default;

    /// Build the Vulkan pipeline object. Returns a minimal StarPipeline handle.
    StarPipeline createPipeline(vk::Device device, RenderResourceDependencies &deps);

    PipelineType getType() const
    {
        return m_type;
    }
    vk::PipelineLayout getLayout() const
    {
        return m_layout;
    }
    const std::vector<Handle> &getShaders() const
    {
        return m_shaders;
    }
    const GraphicsOverrides &getGraphicsOverrides() const
    {
        return m_graphics;
    }

  private:
    struct SpecializedStages
    {
        std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
        std::vector<vk::SpecializationInfo> specializationInfos;
    };

    std::vector<Handle> m_shaders;
    GraphicsOverrides m_graphics;
    absl::flat_hash_map<Shader_Stage, star::StarPipelineConstants> m_pipelineConstants;
    vk::PipelineLayout m_layout{VK_NULL_HANDLE};
    PipelineType m_type{PipelineType::Graphics};

    vk::Pipeline buildGraphics(vk::Device device, RenderResourceDependencies &deps,
                               const SpecializedStages &specializedStages);
    vk::Pipeline buildCompute(vk::Device device, RenderResourceDependencies &deps,
                              const SpecializedStages &specializedStages);
    SpecializedStages processPipelineConstants(
        const std::vector<vk::PipelineShaderStageCreateInfo> &workingShaderInfos);
};

class StarPipeline
{
  public:
    StarPipeline() = default;
    StarPipeline(vk::Pipeline pipeline, PipelineType type) : m_pipeline(pipeline), m_type(type)
    {
    }

    StarPipeline(const StarPipeline &) = delete;
    StarPipeline &operator=(const StarPipeline &) = delete;
    StarPipeline(StarPipeline &&other) noexcept : m_pipeline(other.m_pipeline), m_type(other.m_type)
    {
        other.m_pipeline = VK_NULL_HANDLE;
    }
    StarPipeline &operator=(StarPipeline &&other) noexcept
    {
        if (this != &other)
        {
            m_pipeline = other.m_pipeline;
            m_type = other.m_type;
            other.m_pipeline = VK_NULL_HANDLE;
        }
        return *this;
    }

    bool isRenderReady() const
    {
        return m_pipeline != VK_NULL_HANDLE;
    }
    vk::Pipeline getVulkanPipeline() const
    {
        return m_pipeline;
    }
    void bind(vk::CommandBuffer &commandBuffer) const;
    void destroy(vk::Device device);

    /// Back-compat alias so existing references to StarPipeline::RenderResourceDependencies compile.
    using RenderResourceDependencies = PipelineProvider::RenderResourceDependencies;

  private:
    vk::Pipeline m_pipeline{VK_NULL_HANDLE};
    PipelineType m_type{PipelineType::Graphics};
};

} // namespace star