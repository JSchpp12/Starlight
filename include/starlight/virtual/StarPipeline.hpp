#pragma once

#include "StarShader.hpp"
#include "VulkanVertex.hpp"
#include "core/device/StarDevice.hpp"
#include "core/renderer/RenderingTargetInfo.hpp"

#include <star_common/Handle.hpp>
#include <vulkan/vulkan.hpp>

#include <memory>
#include <optional>
#include <string>
#include <utility>
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
        std::vector<std::pair<star::StarShader, std::shared_ptr<std::vector<uint32_t>>>> compiledShaders;
        core::renderer::RenderingTargetInfo renderingTargetInfo;
        vk::Extent2D swapChainExtent;
    };

    PipelineProvider() = default;

    /// Default graphics pipeline: engine-default state + the global VulkanVertex layout.
    PipelineProvider(std::vector<Handle> shaders, vk::PipelineLayout layout)
        : m_type(PipelineType::Graphics), m_shaders(std::move(shaders)), m_layout(layout)
    {
    }

    /// Graphics pipeline with custom vertex input / state overrides.
    PipelineProvider(std::vector<Handle> shaders, vk::PipelineLayout layout, GraphicsOverrides overrides)
        : m_type(PipelineType::Graphics), m_shaders(std::move(shaders)), m_layout(layout),
          m_graphics(std::move(overrides))
    {
    }

    /// Compute pipeline (single compute shader).
    PipelineProvider(Handle computeShader, vk::PipelineLayout layout)
        : m_type(PipelineType::Compute), m_shaders{std::move(computeShader)}, m_layout(layout)
    {
    }

    /// Build the Vulkan pipeline object. Returns a minimal StarPipeline handle.
    StarPipeline build(vk::Device device, const RenderResourceDependencies &deps) const;

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
    PipelineType m_type = PipelineType::Graphics;
    std::vector<Handle> m_shaders;
    vk::PipelineLayout m_layout = VK_NULL_HANDLE;
    GraphicsOverrides m_graphics;

    vk::Pipeline buildGraphics(vk::Device device, const RenderResourceDependencies &deps) const;
    vk::Pipeline buildCompute(vk::Device device, const RenderResourceDependencies &deps) const;

    static vk::ShaderModule createShaderModule(vk::Device &device, const std::vector<uint32_t> &sourceCode);

    static void processShaders(vk::Device &device, const RenderResourceDependencies &deps, vk::ShaderModule &vertModule,
                               vk::ShaderModule &fragModule, vk::ShaderModule &geoModule);
};

/// Minimal built pipeline handle. Owns the vk::Pipeline and knows its bind
/// point; nothing else is retained after build. Created exclusively by
/// PipelineProvider::build.
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
    vk::Pipeline m_pipeline = VK_NULL_HANDLE;
    PipelineType m_type = PipelineType::Graphics;
};

} // namespace star