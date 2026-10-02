#include "starlight/command/pipeline/CreatePipeline.hpp"

#include "starlight/core/Exceptions.hpp"

#include <unordered_set>

namespace star::command::pipeline
{
CreatePipeline &CreatePipeline::addShaderHandle(Handle handle)
{
    if (m_numShaderHandles >= shader::MaxShadersPerPipeline)
    {
        STAR_THROW("CreatePipeline cannot hold more than MaxShadersPerPipeline shader handles");
    }

    m_shaderHandles[m_numShaderHandles++] = std::move(handle);
    return *this;
}

CreatePipeline &CreatePipeline::addShaderRequest(shader::ShaderBuildRequest request)
{
    if (!request.isValid())
    {
        STAR_THROW("Cannot add an invalid shader build request to a CreatePipeline");
    }

    if (m_numShaderRequests >= shader::MaxShadersPerPipeline)
    {
        STAR_THROW("CreatePipeline cannot hold more than MaxShadersPerPipeline shader requests");
    }

    m_shaderRequests[m_numShaderRequests++] = std::move(request);
    return *this;
}

void CreatePipeline::validate() const
{
    const std::size_t numShaders = getShaderCount();

    if (m_type == PipelineType::Compute)
    {
        if (numShaders != 1)
        {
            STAR_THROW("A compute pipeline requires exactly one compute shader");
        }

        for (std::size_t i = 0; i < m_numShaderRequests; ++i)
        {
            if (m_shaderRequests[i].stage != Shader_Stage::compute)
            {
                STAR_THROW("A compute pipeline only accepts compute shaders");
            }
        }

        for (std::size_t i = 0; i < m_numShaderHandles; ++i)
        {
            if (!m_shaderHandles[i].isInitialized())
            {
                STAR_THROW("A compute pipeline requires an initialized shader handle");
            }
        }
    }
    else
    {
        if (numShaders == 0)
        {
            STAR_THROW("A graphics pipeline requires at least one shader");
        }

        std::unordered_set<Shader_Stage> stages;
        for (std::size_t i = 0; i < m_numShaderRequests; ++i)
        {
            if (!stages.insert(m_shaderRequests[i].stage).second)
            {
                STAR_THROW("A graphics pipeline accepts at most one shader per stage");
            }
        }

        if (m_swapChainExtent.width == 0 || m_swapChainExtent.height == 0)
        {
            STAR_THROW("A graphics pipeline requires a valid swap chain extent");
        }

        if (m_renderingTargetInfo.colorAttachmentFormats.empty() &&
            !m_renderingTargetInfo.depthAttachmentFormat.has_value() &&
            !m_renderingTargetInfo.stencilAttachmentFormat.has_value())
        {
            STAR_THROW("A graphics pipeline requires rendering target info");
        }
    }

    if (m_layout == VK_NULL_HANDLE)
    {
        STAR_THROW("A pipeline requires a valid pipeline layout");
    }
}
} // namespace star::command::pipeline
