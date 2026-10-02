#pragma once

#include "starlight/command/shader/ShaderBuildRequest.hpp"

#include <StarPipeline.hpp>
#include <core/renderer/RenderingTargetInfo.hpp>
#include <star_common/Handle.hpp>
#include <star_common/IServiceCommandWithReply.hpp>

#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vulkan/vulkan.hpp>

namespace star::command::pipeline
{
namespace create_pipeline
{
inline constexpr const char *GetUniqueTypeName()
{
    return "star::command::pipeline::create_pipeline";
}
} // namespace create_pipeline

/// Request that a graphics or compute pipeline be built asynchronously. Shaders
/// may be supplied either as already resolved handles or as raw build requests
/// that the service resolves first. The reply holds a pending pipeline handle;
/// it does not wait for PipelineReady.
struct CreatePipeline : public star::common::IServiceCommandWithReply<Handle>
{
    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return create_pipeline::GetUniqueTypeName();
    }

    CreatePipeline &setComputePipeline()
    {
        m_type = PipelineType::Compute;
        return *this;
    }

    CreatePipeline &setGraphicsPipeline()
    {
        m_type = PipelineType::Graphics;
        return *this;
    }

    CreatePipeline &addShaderHandle(Handle handle);

    CreatePipeline &addShaderRequest(shader::ShaderBuildRequest request);

    CreatePipeline &setPipelineLayout(vk::PipelineLayout layout)
    {
        m_layout = layout;
        return *this;
    }

    CreatePipeline &setSwapChainExtent(vk::Extent2D extent)
    {
        m_swapChainExtent = extent;
        return *this;
    }

    CreatePipeline &setRenderingTargetInfo(core::renderer::RenderingTargetInfo info)
    {
        m_renderingTargetInfo = std::move(info);
        return *this;
    }

    CreatePipeline &setGraphicsOverrides(GraphicsOverrides overrides)
    {
        m_graphicsOverrides = std::move(overrides);
        return *this;
    }

    /// Validate the pipeline recipe. Throws on an invalid configuration. The
    /// service calls this before touching any manager so an invalid request never
    /// produces a partial resource.
    void validate() const;

    PipelineType getPipelineType() const
    {
        return m_type;
    }

    std::span<const Handle> getShaderHandles() const
    {
        return {m_shaderHandles.data(), m_numShaderHandles};
    }

    std::span<const shader::ShaderBuildRequest> getShaderRequests() const
    {
        return {m_shaderRequests.data(), m_numShaderRequests};
    }

    std::size_t getShaderCount() const
    {
        return m_numShaderHandles + m_numShaderRequests;
    }

    vk::PipelineLayout getPipelineLayout() const
    {
        return m_layout;
    }

    vk::Extent2D getSwapChainExtent() const
    {
        return m_swapChainExtent;
    }

    const core::renderer::RenderingTargetInfo &getRenderingTargetInfo() const
    {
        return m_renderingTargetInfo;
    }

    const std::optional<GraphicsOverrides> &getGraphicsOverrides() const
    {
        return m_graphicsOverrides;
    }

  private:
    PipelineType m_type = PipelineType::Graphics;
    std::array<Handle, shader::MaxShadersPerPipeline> m_shaderHandles{};
    std::array<shader::ShaderBuildRequest, shader::MaxShadersPerPipeline> m_shaderRequests{};
    std::size_t m_numShaderHandles = 0;
    std::size_t m_numShaderRequests = 0;
    vk::PipelineLayout m_layout = VK_NULL_HANDLE;
    vk::Extent2D m_swapChainExtent{};
    core::renderer::RenderingTargetInfo m_renderingTargetInfo{};
    std::optional<GraphicsOverrides> m_graphicsOverrides;
};
} // namespace star::command::pipeline
