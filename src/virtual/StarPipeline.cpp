#include "StarPipeline.hpp"

#include "job/tasks/TaskFactory.hpp"

star::StarPipeline::~StarPipeline()
{
	assert(!m_pipeline && "Pipeline was not properly destroyed"); 
}

bool star::StarPipeline::isRenderReady(core::device::DeviceContext &context){
    assert(m_shaders.size() > 0 && "Pipeline not properly initialized"); 


    if (m_pipeline != VK_NULL_HANDLE)
        return true; 

    for (const auto &sh : m_shaders){
        if (!context.getShaderManager().isReady(sh))
            return false; 
    }

    //going to go ahead and build
    prepRender(context); 
    return true; 
}

void star::StarPipeline::init(core::device::DeviceContext &context, vk::PipelineLayout pipelineLayout, RenderingTargetInfo renderingInfo)
{
    m_shaders = submitShaders(context);
	// prepRender(context); 
}

void star::StarPipeline::prepRender(core::device::DeviceContext &context){
    assert(m_shaders.size() > 0 && "Not prepared correctly"); 

    m_pipeline = buildPipeline(context); 
}

void star::StarPipeline::cleanupRender(core::device::DeviceContext &context)
{
    context.getDevice().getVulkanDevice().destroyPipeline(m_pipeline);
	m_pipeline = nullptr; 
}

bool star::StarPipeline::isSame(StarPipeline &compPipe)
{
    return (m_hash == compPipe.m_hash);
}

vk::ShaderModule star::StarPipeline::CreateShaderModule(core::device::StarDevice &device, const std::vector<uint32_t> &sourceCode)
{
    vk::ShaderModuleCreateInfo createInfo{};
    createInfo.sType = vk::StructureType::eShaderModuleCreateInfo;
    createInfo.codeSize = 4 * sourceCode.size();
    createInfo.pCode = sourceCode.data();

    VkShaderModule shaderModule = device.getVulkanDevice().createShaderModule(createInfo);
    if (!shaderModule)
    {
        throw std::runtime_error("failed to create shader module");
    }
    return shaderModule;
}