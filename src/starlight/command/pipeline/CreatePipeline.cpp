#include "starlight/command/pipeline/CreatePipeline.hpp"

#include "starlight/core/Exceptions.hpp"

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

void CreatePipeline::validate() const
{
    const std::size_t numShaders = getShaderCount();

    if (m_type == PipelineType::Compute)
    {
        if (numShaders != 1)
        {
            STAR_THROW("A compute pipeline requires exactly one compute shader");
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
